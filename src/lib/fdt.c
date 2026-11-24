// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file fdt.c
 * @brief Flattened device tree parser (see fdt.h)
 * @ingroup lib
 *
 * Offsets called @c off below are byte offsets into the structure block
 * (blob + off_dt_struct). They never leave this file: callers only see
 * struct fdt_node.
 */

#include "lib/fdt.h"

#include "lib/string.h"

#include <kernel/error.h>

#define FDT_MAGIC                 0xD00DFEEDu ///< Header magic (spec 5.2)
#define FDT_MIN_VERSION           17u         ///< Oldest format version understood
#define FDT_LAST_COMP_VERSION     17u         ///< Newest format version this parser implements

#define FDT_HEADER_SIZE           40u ///< Size of the v17 header (ten u32 fields)
#define FDT_RSV_ENTRY_SIZE        16u ///< Size of one reservation entry (two u64)
#define FDT_PROP_HEADER_LEN       12u ///< FDT_PROP token + len + nameoff

// Structure block tokens (spec 5.4.1)
#define FDT_BEGIN_NODE            0x1u ///< Start of a node, followed by its name
#define FDT_END_NODE              0x2u ///< End of a node
#define FDT_PROP                  0x3u ///< Property: len, nameoff, then len bytes of value
#define FDT_NOP                   0x4u ///< Ignored
#define FDT_END                   0x9u ///< End of the structure block

#define FDT_DEFAULT_ADDRESS_CELLS 2u ///< \#address-cells when the property is absent (spec 2.3.5)
#define FDT_DEFAULT_SIZE_CELLS    1u ///< \#size-cells when the property is absent (spec 2.3.5)
#define FDT_MAX_CELLS             2u ///< Most cells that fit in a uint64_t

/// @brief Header fields in native endianness (spec 5.2)
struct fdt_header
{
	uint32_t magic;             ///< Must be FDT_MAGIC
	uint32_t totalsize;         ///< Size of the whole blob in bytes
	uint32_t off_dt_struct;     ///< Offset of the structure block
	uint32_t off_dt_strings;    ///< Offset of the strings block
	uint32_t off_mem_rsvmap;    ///< Offset of the memory reservation block
	uint32_t version;           ///< Format version
	uint32_t last_comp_version; ///< Oldest version this blob is compatible with
	uint32_t boot_cpuid_phys;   ///< Physical ID of the boot CPU
	uint32_t size_dt_strings;   ///< Size of the strings block in bytes
	uint32_t size_dt_struct;    ///< Size of the structure block in bytes
};

static struct fdt_header s_header; ///< Header of the selected blob
static const uint8_t*    s_blob;   ///< Selected blob; NULL until fdt_init() succeeds

/// @brief Rounds @p v up to a multiple of 4 (tokens are 4-byte aligned, spec 5.6)
static inline uint32_t align4(uint32_t v)
{
	return (v + 3u) & ~3u;
}

/// @brief Reads a big-endian u32 at any alignment
static uint32_t read_be32(const void* p)
{
	uint32_t val;
	// memcpy instead of a cast: no strict-aliasing issue, and with -mstrict-align the
	// compiler never emits an unaligned load (which faults while the MMU is off)
	__builtin_memcpy(&val, p, sizeof(val));
	return __builtin_bswap32(val);
}

/// @brief Reads a big-endian u64 at any alignment
static uint64_t read_be64(const void* p)
{
	uint64_t val;
	__builtin_memcpy(&val, p, sizeof(val));
	return __builtin_bswap64(val);
}

/// @brief Start of the structure block
static inline const uint8_t* struct_base()
{
	return s_blob + s_header.off_dt_struct;
}

/// @brief String at @p nameoff in the strings block, or NULL if out of bounds
static const char* get_string(uint32_t nameoff)
{
	if (nameoff >= s_header.size_dt_strings)
		return NULL;
	// fdt_init() checked that the block ends with a NUL, so this string is terminated
	return (const char*) (s_blob + s_header.off_dt_strings + nameoff);
}

uint32_t fdt_read_u32(const void* p)
{
	return read_be32(p);
}

uint64_t fdt_read_cells(const void* p, uint32_t cell_count)
{
	uint64_t       v     = 0;
	const uint8_t* bytes = p;
	for (uint32_t i = 0; i < cell_count; i++)
		v = (v << 32) | read_be32(bytes + 4 * i);
	return v;
}

/**
 * @brief Checks that the block [@p off, @p off + @p size) lies inside a blob of @p total bytes.
 *
 * Written with subtractions so that no addition can wrap around.
 */
static bool block_fits(uint32_t off, uint32_t size, uint32_t total)
{
	return off >= FDT_HEADER_SIZE && off <= total && size <= total - off;
}

/**
 * @brief Reads and checks the header of @p blob into s_header.
 * @retval SUCCESS        The header is valid
 * @retval -ERROR_GENERIC Bad magic, unsupported version or a block out of bounds
 */
static int parse_header(const uint8_t* blob)
{
	const uint8_t* p = blob;

	s_header.magic             = read_be32(p + 0);
	s_header.totalsize         = read_be32(p + 4);
	s_header.off_dt_struct     = read_be32(p + 8);
	s_header.off_dt_strings    = read_be32(p + 12);
	s_header.off_mem_rsvmap    = read_be32(p + 16);
	s_header.version           = read_be32(p + 20);
	s_header.last_comp_version = read_be32(p + 24);
	s_header.boot_cpuid_phys   = read_be32(p + 28);
	s_header.size_dt_strings   = read_be32(p + 32);
	s_header.size_dt_struct    = read_be32(p + 36);

	if (s_header.magic != FDT_MAGIC)
		return -ERROR_GENERIC;
	if (s_header.version < FDT_MIN_VERSION || s_header.last_comp_version > FDT_LAST_COMP_VERSION)
		return -ERROR_GENERIC;
	if (s_header.totalsize < FDT_HEADER_SIZE)
		return -ERROR_GENERIC;

	if (!block_fits(s_header.off_dt_struct, s_header.size_dt_struct, s_header.totalsize))
		return -ERROR_GENERIC;
	if (!block_fits(s_header.off_dt_strings, s_header.size_dt_strings, s_header.totalsize))
		return -ERROR_GENERIC;

	// The reservation block comes before the structure block and has room for at
	// least the (0, 0) terminator
	if (s_header.off_mem_rsvmap < FDT_HEADER_SIZE || s_header.off_mem_rsvmap >= s_header.off_dt_struct || s_header.off_dt_struct - s_header.off_mem_rsvmap < FDT_RSV_ENTRY_SIZE)
		return -ERROR_GENERIC;

	// Alignment (spec 5.6): the u64 reads of the reservation block rely on 8, the token
	// reads on 4
	if (s_header.off_mem_rsvmap % 8 != 0 || s_header.off_dt_struct % 4 != 0 || s_header.size_dt_struct % 4 != 0)
		return -ERROR_GENERIC;

	// A strings block that ends with a NUL makes every string in it terminated, so
	// property names can be compared without further bounds checks
	if (s_header.size_dt_strings == 0 || blob[s_header.off_dt_strings + s_header.size_dt_strings - 1] != '\0')
		return -ERROR_GENERIC;

	return SUCCESS;
}

int fdt_init(uintptr_t blob_pa)
{
	s_blob = NULL;
	if (blob_pa == 0 || blob_pa % 8 != 0) // spec 5.1: the blob is 8-byte aligned
		return -ERROR_INVALID_ARG;

	const uint8_t* blob = (const uint8_t*) blob_pa;
	int            ret  = parse_header(blob);
	if (ret != SUCCESS)
		return ret;

	s_blob = blob;
	return SUCCESS;
}

uintptr_t fdt_get_blob_address()
{
	return (uintptr_t) s_blob;
}

size_t fdt_get_blob_size()
{
	return s_blob ? s_header.totalsize : 0;
}

int fdt_get_reserve(uint32_t idx, struct fdt_reserve_entry* out)
{
	if (!s_blob)
		return -ERROR_NO_DEVICE;
	if (!out)
		return -ERROR_INVALID_ARG;

	// Entries 0..idx must all lie before the structure block. 64-bit math: idx * 16 can't wrap
	uint64_t off = (uint64_t) s_header.off_mem_rsvmap + (uint64_t) idx * FDT_RSV_ENTRY_SIZE;
	if (off + FDT_RSV_ENTRY_SIZE > s_header.off_dt_struct)
		return -ERROR_GENERIC; // ran out of block without meeting the terminator

	const uint8_t* e = s_blob + off;
	out->address     = read_be64(e);
	out->size        = read_be64(e + 8);

	if (out->address == 0 && out->size == 0)
		return -ERROR_NOT_FOUND; // the terminator
	return SUCCESS;
}

/**
 * @brief Reads the token at @p off and finds the offset of the one after it.
 *
 * @param off   Offset of a token in the structure block.
 * @param token The token found at @p off.
 * @param next  Offset of the following token (unchanged for FDT_END).
 * @retval SUCCESS            @p token and @p next are valid
 * @retval -ERROR_INVALID_ARG @p off is misaligned or out of the block
 * @retval -ERROR_GENERIC     Unknown token, unterminated name or a property running past the block
 */
static int next_token(uint32_t off, uint32_t* token, uint32_t* next)
{
	const uint32_t size = s_header.size_dt_struct;
	if (off % 4 != 0 || size < 4 || off > size - 4)
		return -ERROR_INVALID_ARG;

	const uint8_t* base = struct_base();
	uint32_t       tok  = read_be32(base + off);

	switch (tok)
	{
	case FDT_BEGIN_NODE:
	{
		uint32_t i = off + 4; // the name
		while (i < size && base[i] != '\0')
			i++;
		if (i >= size)
			return -ERROR_GENERIC; // no NUL before the end of the block
		*next = align4(i + 1);
		break;
	}

	case FDT_PROP:
	{
		if (size - off < FDT_PROP_HEADER_LEN)
			return -ERROR_GENERIC;
		uint32_t len = read_be32(base + off + 4);
		if (len > size - off - FDT_PROP_HEADER_LEN)
			return -ERROR_GENERIC; // value runs past the block
		*next = align4(off + FDT_PROP_HEADER_LEN + len);
		break;
	}

	case FDT_END_NODE:
	case FDT_NOP:
		*next = off + 4;
		break;

	case FDT_END:
		break; // nothing follows

	default:
		return -ERROR_GENERIC;
	}

	*token = tok;
	return SUCCESS;
}

/**
 * @brief Checks that @p node is a node and finds the first token after its name.
 * @param node  Node to check.
 * @param first Offset of the first token after the name (a property, child or END_NODE).
 * @return #SUCCESS, -ERROR_NO_DEVICE, -ERROR_INVALID_ARG if @p node isn't a node,
 *         or -ERROR_GENERIC.
 */
static int node_body(struct fdt_node node, uint32_t* first)
{
	if (!s_blob)
		return -ERROR_NO_DEVICE;

	uint32_t token;
	int      ret = next_token(node.off, &token, first);
	if (ret != SUCCESS)
		return ret;
	if (token != FDT_BEGIN_NODE)
		return -ERROR_INVALID_ARG;
	return SUCCESS;
}

/// @brief From @p off, skips NOP tokens; @p token / @p next describe the first other token
static int skip_nops(uint32_t* off, uint32_t* token, uint32_t* next)
{
	for (;;)
	{
		int ret = next_token(*off, token, next);
		if (ret != SUCCESS)
			return ret == -ERROR_INVALID_ARG ? -ERROR_GENERIC : ret; // ran off the block
		if (*token != FDT_NOP)
			return SUCCESS;
		*off = *next;
	}
}

int fdt_get_root(struct fdt_node* out)
{
	if (!s_blob)
		return -ERROR_NO_DEVICE;
	if (!out)
		return -ERROR_INVALID_ARG;

	// The structure block starts with the root's BEGIN_NODE, possibly after NOPs
	uint32_t off = 0, token, next;
	int      ret = skip_nops(&off, &token, &next);
	if (ret != SUCCESS)
		return ret;
	if (token != FDT_BEGIN_NODE)
		return -ERROR_GENERIC;

	out->off = off;
	return SUCCESS;
}

int fdt_first_subnode(struct fdt_node parent, struct fdt_node* out)
{
	if (!out)
		return -ERROR_INVALID_ARG;

	uint32_t off;
	int      ret = node_body(parent, &off);
	if (ret != SUCCESS)
		return ret;

	// Properties come before children (spec 5.4.2): skip them
	for (;;)
	{
		uint32_t token, next;
		ret = skip_nops(&off, &token, &next);
		if (ret != SUCCESS)
			return ret;

		switch (token)
		{
		case FDT_PROP:
			off = next;
			break;
		case FDT_BEGIN_NODE:
			out->off = off;
			return SUCCESS;
		case FDT_END_NODE:
			return -ERROR_NOT_FOUND; // no children
		default:
			return -ERROR_GENERIC; // FDT_END inside a node
		}
	}
}

int fdt_next_subnode(struct fdt_node node, struct fdt_node* out)
{
	if (!out)
		return -ERROR_INVALID_ARG;

	uint32_t off;
	int      ret = node_body(node, &off);
	if (ret != SUCCESS)
		return ret;

	// Skip the rest of node's subtree. We're already inside node, so depth starts at 1
	// and reaches 0 right after node's own END_NODE
	uint32_t depth = 1;
	while (depth > 0)
	{
		uint32_t token, next;
		ret = skip_nops(&off, &token, &next);
		if (ret != SUCCESS)
			return ret;

		if (token == FDT_BEGIN_NODE)
			depth++;
		else if (token == FDT_END_NODE)
			depth--;
		else if (token == FDT_END)
			return -ERROR_GENERIC; // the tree ended inside a node
		off = next;
	}

	// What follows node's END_NODE: a sibling, or the parent's END_NODE
	uint32_t token, next;
	ret = skip_nops(&off, &token, &next);
	if (ret != SUCCESS)
		return ret;

	switch (token)
	{
	case FDT_BEGIN_NODE:
		out->off = off;
		return SUCCESS;
	case FDT_END_NODE:
	case FDT_END: // after the root
		return -ERROR_NOT_FOUND;
	default:
		return -ERROR_GENERIC; // a property after a child node
	}
}

const char* fdt_get_node_name(struct fdt_node node)
{
	uint32_t first;
	if (node_body(node, &first) != SUCCESS)
		return NULL;
	// next_token() checked that the name is NUL-terminated inside the block
	return (const char*) (struct_base() + node.off + 4);
}

bool fdt_is_node_named(struct fdt_node node, const char* name)
{
	const char* node_name = fdt_get_node_name(node);
	if (!node_name || !name)
		return false;

	size_t len = strlen(name);
	if (strncmp(node_name, name, len) != 0)
		return false;
	// Exact match, or name without a unit address followed by "@unit"
	return node_name[len] == '\0' || node_name[len] == '@';
}

int fdt_find_subnode(struct fdt_node parent, const char* name, struct fdt_node* out)
{
	if (!name || !out)
		return -ERROR_INVALID_ARG;

	struct fdt_node child;
	int             ret = fdt_first_subnode(parent, &child);
	for (; ret == SUCCESS; ret = fdt_next_subnode(child, &child))
	{
		if (fdt_is_node_named(child, name))
		{
			*out = child;
			return SUCCESS;
		}
	}
	return ret; // -ERROR_NOT_FOUND at the end, or a real error
}

int fdt_get_prop(struct fdt_node node, const char* name, struct fdt_prop* out)
{
	if (!name || !out)
		return -ERROR_INVALID_ARG;

	uint32_t off;
	int      ret = node_body(node, &off);
	if (ret != SUCCESS)
		return ret;

	const uint8_t* base = struct_base();
	for (;;)
	{
		uint32_t token, next;
		ret = skip_nops(&off, &token, &next);
		if (ret != SUCCESS)
			return ret;
		if (token != FDT_PROP)
			return -ERROR_NOT_FOUND; // reached a child or the node's end: no more properties

		const char* prop_name = get_string(read_be32(base + off + 8));
		if (!prop_name)
			return -ERROR_GENERIC;
		if (strcmp(prop_name, name) == 0)
		{
			out->len  = read_be32(base + off + 4);
			out->data = base + off + FDT_PROP_HEADER_LEN;
			return SUCCESS;
		}
		off = next;
	}
}

bool fdt_is_node_enabled(struct fdt_node node)
{
	struct fdt_prop status;
	int             ret = fdt_get_prop(node, "status", &status);
	if (ret == -ERROR_NOT_FOUND)
		return true; // absent means "okay"
	if (ret != SUCCESS || status.len == 0)
		return false;

	const char* s = status.data;
	if (s[status.len - 1] != '\0')
		return false; // not a string
	// "ok" is a legacy spelling some trees still use
	return strcmp(s, "okay") == 0 || strcmp(s, "ok") == 0;
}

/// @brief Reads a single-u32 property @p name of @p node, or @p fallback if absent
static int get_u32_prop(struct fdt_node node, const char* name, uint32_t fallback, uint32_t* out)
{
	if (!out)
		return -ERROR_INVALID_ARG;

	struct fdt_prop prop;
	int             ret = fdt_get_prop(node, name, &prop);
	if (ret == -ERROR_NOT_FOUND)
	{
		*out = fallback;
		return SUCCESS;
	}
	if (ret != SUCCESS)
		return ret;
	if (prop.len != 4)
		return -ERROR_GENERIC;

	*out = read_be32(prop.data);
	return SUCCESS;
}

int fdt_get_address_cells(struct fdt_node node, uint32_t* out)
{
	return get_u32_prop(node, "#address-cells", FDT_DEFAULT_ADDRESS_CELLS, out);
}

int fdt_get_size_cells(struct fdt_node node, uint32_t* out)
{
	return get_u32_prop(node, "#size-cells", FDT_DEFAULT_SIZE_CELLS, out);
}

int fdt_get_reg(struct fdt_node node, struct fdt_node parent, uint32_t idx, uint64_t* addr, uint64_t* size)
{
	if (!addr || !size)
		return -ERROR_INVALID_ARG;

	// The cell counts that describe node's reg live in its parent (spec 2.3.5)
	uint32_t address_cells, size_cells;
	int      ret = fdt_get_address_cells(parent, &address_cells);
	if (ret != SUCCESS)
		return ret;
	ret = fdt_get_size_cells(parent, &size_cells);
	if (ret != SUCCESS)
		return ret;
	if (address_cells == 0 || address_cells > FDT_MAX_CELLS || size_cells > FDT_MAX_CELLS)
		return -ERROR_GENERIC;

	struct fdt_prop reg;
	ret = fdt_get_prop(node, "reg", &reg);
	if (ret != SUCCESS)
		return ret;

	uint32_t tuple_size = (address_cells + size_cells) * 4;
	if (reg.len % tuple_size != 0)
		return -ERROR_GENERIC; // corrupt, or the cell counts don't belong to this node
	if (idx >= reg.len / tuple_size)
		return -ERROR_NOT_FOUND;

	const uint8_t* tuple = (const uint8_t*) reg.data + idx * tuple_size;
	*addr                = fdt_read_cells(tuple, address_cells);
	*size                = fdt_read_cells(tuple + address_cells * 4, size_cells);
	return SUCCESS;
}

int fdt_translate_address(struct fdt_node bus, struct fdt_node bus_parent, uint64_t child_addr, uint64_t* out)
{
	if (!out)
		return -ERROR_INVALID_ARG;

	struct fdt_prop ranges;
	int             ret = fdt_get_prop(bus, "ranges", &ranges);
	if (ret != SUCCESS)
		return ret; // -ERROR_NOT_FOUND: no ranges, the bus isn't translatable
	if (ranges.len == 0)
	{
		*out = child_addr; // empty ranges: identity mapping
		return SUCCESS;
	}

	// Child addresses and sizes use the bus's own cell counts, parent addresses the
	// parent's (spec 2.3.8)
	uint32_t child_cells, parent_cells, size_cells;
	if ((ret = fdt_get_address_cells(bus, &child_cells)) != SUCCESS)
		return ret;
	if ((ret = fdt_get_address_cells(bus_parent, &parent_cells)) != SUCCESS)
		return ret;
	if ((ret = fdt_get_size_cells(bus, &size_cells)) != SUCCESS)
		return ret;
	if (child_cells == 0 || child_cells > FDT_MAX_CELLS || parent_cells == 0 || parent_cells > FDT_MAX_CELLS ||
	    size_cells == 0 || size_cells > FDT_MAX_CELLS)
		return -ERROR_GENERIC;

	uint32_t entry_size = (child_cells + parent_cells + size_cells) * 4;
	if (ranges.len % entry_size != 0)
		return -ERROR_GENERIC;

	const uint8_t* entry = ranges.data;
	for (uint32_t i = 0; i < ranges.len / entry_size; i++, entry += entry_size)
	{
		uint64_t child_base  = fdt_read_cells(entry, child_cells);
		uint64_t parent_base = fdt_read_cells(entry + child_cells * 4, parent_cells);
		uint64_t size        = fdt_read_cells(entry + (child_cells + parent_cells) * 4, size_cells);

		// child_addr - child_base < size also rejects child_addr < child_base (it wraps)
		if (child_addr >= child_base && child_addr - child_base < size)
		{
			*out = parent_base + (child_addr - child_base);
			return SUCCESS;
		}
	}
	return -ERROR_NOT_FOUND;
}
