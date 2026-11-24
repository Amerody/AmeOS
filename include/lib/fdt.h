// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#pragma once

/**
 * @file fdt.h
 * @brief Read-only, non-allocating parser for a flattened device tree (DTB).
 * @ingroup lib
 *
 * Works in place on the blob the firmware passes in x0, so it can run before
 * any memory allocator exists. The format is described in the Devicetree
 * Specification, chapter 5 ("Flattened Devicetree (DTB) Format"); the meaning
 * of the standard properties (\#address-cells, \#size-cells, reg, status) in
 * chapter 2.3.
 *
 * Usage: call fdt_init() once with the blob's address. Every other function
 * works on that blob and returns -ERROR_NO_DEVICE until fdt_init() succeeded.
 * Nodes are reached from fdt_get_root() with fdt_first_subnode() /
 * fdt_next_subnode(); properties with fdt_get_prop().
 *
 * Error convention (all @c int functions):
 *  - #SUCCESS: the result was written to the output parameter;
 *  - -ERROR_NOT_FOUND: no such node, property or entry (the normal end of an
 *    iteration);
 *  - -ERROR_INVALID_ARG: a NULL pointer, or a handle that isn't a node;
 *  - -ERROR_GENERIC: the blob is malformed;
 *  - -ERROR_NO_DEVICE: fdt_init() hasn't succeeded.
 *
 * Every value returned (addresses, sizes, cell counts) is already in native
 * endianness, except the raw bytes behind fdt_prop::data, which callers decode
 * with fdt_read_u32() / fdt_read_cells().
 *
 * Not implemented yet: multi-level @c ranges translation, phandles, interrupts, aliases.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Handle to a node of the tree.
	 *
	 * Only obtained from fdt_get_root(), fdt_first_subnode(), fdt_next_subnode()
	 * or fdt_find_subnode(); don't build one by hand. Valid for the blob given
	 * to fdt_init().
	 */
	struct fdt_node
	{
		uint32_t off; ///< Offset of the node's FDT_BEGIN_NODE token in the structure block
	};

	/**
	 * @brief A property value: the raw bytes inside the blob.
	 *
	 * The bytes are big-endian and 4-byte aligned. Strings are NUL-terminated;
	 * a string list (e.g. @c compatible) is several strings back to back and
	 * @c len covers all of them.
	 */
	struct fdt_prop
	{
		const void* data; ///< First byte of the value; never NULL on success
		uint32_t    len;  ///< Length in bytes; 0 for an empty property such as @c ranges;
	};

	/// @brief One entry of the memory reservation block (spec 5.3), in native endianness
	struct fdt_reserve_entry
	{
		uint64_t address; ///< Physical address of the reserved range
		uint64_t size;    ///< Size of the reserved range in bytes
	};

	/**
	 * @brief Checks the blob's header and makes it the blob all other functions use.
	 *
	 * Checks the magic, the version and that every block lies inside the blob.
	 * On failure no blob is selected, not even one from an earlier call.
	 *
	 * @param blob_pa Physical address of the DTB, as passed by the firmware.
	 * @retval SUCCESS            The header is valid
	 * @retval -ERROR_INVALID_ARG @p blob_pa is 0 or not 8-byte aligned
	 * @retval -ERROR_GENERIC     The header is malformed or the version unsupported
	 */
	int fdt_init(uintptr_t blob_pa);

	/**
	 * @brief Address of the blob selected by fdt_init().
	 * @return The address, or 0 if fdt_init() hasn't succeeded.
	 */
	uintptr_t fdt_get_blob_address();

	/**
	 * @brief Size of the whole blob (the header's @c totalsize), for reserving its memory.
	 * @return The size in bytes, or 0 if fdt_init() hasn't succeeded.
	 */
	size_t fdt_get_blob_size();

	/**
	 * @brief Reads entry @p idx of the memory reservation block (spec 5.3).
	 *
	 * Iterate with idx = 0, 1, ... until -ERROR_NOT_FOUND.
	 *
	 * @param idx Index of the entry, from 0.
	 * @param out Entry, converted to native endianness.
	 * @retval SUCCESS          @p out holds entry @p idx
	 * @retval -ERROR_NOT_FOUND @p idx is at or past the (0, 0) terminator
	 * @retval -ERROR_GENERIC   The block has no terminator before the structure block
	 */
	int fdt_get_reserve(uint32_t idx, struct fdt_reserve_entry* out);

	/**
	 * @brief Gets the root node ("/").
	 * @param out The root node.
	 * @return #SUCCESS, or a negative error (see the file description).
	 */
	int fdt_get_root(struct fdt_node* out);

	/**
	 * @brief Gets the first child of @p parent.
	 * @param parent Node whose children to visit.
	 * @param out    The first child.
	 * @retval SUCCESS          @p out is the first child
	 * @retval -ERROR_NOT_FOUND @p parent has no children
	 * @return Other negative errors: see the file description.
	 */
	int fdt_first_subnode(struct fdt_node parent, struct fdt_node* out);

	/**
	 * @brief Gets the sibling that follows @p node (skips @p node's whole subtree).
	 *
	 * @p out may point to the variable passed as @p node, so a loop can be written as
	 * @code
	 * for (int r = fdt_first_subnode(p, &n); r == SUCCESS; r = fdt_next_subnode(n, &n))
	 * @endcode
	 *
	 * @param node Current child.
	 * @param out  The next child of the same parent.
	 * @retval SUCCESS          @p out is the next sibling
	 * @retval -ERROR_NOT_FOUND @p node was the last child
	 * @return Other negative errors: see the file description.
	 */
	int fdt_next_subnode(struct fdt_node node, struct fdt_node* out);

	/**
	 * @brief Finds the first child of @p parent matching @p name (see fdt_is_node_named()).
	 * @param parent Node to search the children of.
	 * @param name   Name to match, e.g. "reserved-memory" or "memory".
	 * @param out    The matching child.
	 * @retval SUCCESS          @p out is the first matching child
	 * @retval -ERROR_NOT_FOUND No child matches
	 * @return Other negative errors: see the file description.
	 */
	int fdt_find_subnode(struct fdt_node parent, const char* name, struct fdt_node* out);

	/**
	 * @brief Name of @p node, including its unit address (e.g. "memory@0"). The root's name is "".
	 * @param node Node to name.
	 * @return The name inside the blob, or NULL if @p node isn't a node.
	 */
	const char* fdt_get_node_name(struct fdt_node node);

	/**
	 * @brief Checks a node name, ignoring the unit address unless @p name has one.
	 *
	 * "memory" matches "memory" and "memory@0" but not "memory-controller";
	 * "memory@0" only matches "memory@0" (spec 2.2.1, node names).
	 *
	 * @param node Node to check.
	 * @param name Name to compare with, with or without a unit address.
	 * @return true if @p node's name matches @p name.
	 */
	bool fdt_is_node_named(struct fdt_node node, const char* name);

	/**
	 * @brief Checks the @c status property (spec 2.3.4).
	 * @param node Node to check.
	 * @return true if @c status is absent, "okay" or "ok"; false otherwise
	 *         (e.g. "disabled") or if @p node isn't a node.
	 */
	bool fdt_is_node_enabled(struct fdt_node node);

	/**
	 * @brief Finds a property of @p node by name.
	 *
	 * Only looks at @p node's own properties, never at its children's.
	 *
	 * @param node Node to look in.
	 * @param name Property name, e.g. "reg".
	 * @param out  The property's value.
	 * @retval SUCCESS          @p out is the property (possibly empty: len == 0)
	 * @retval -ERROR_NOT_FOUND @p node has no such property
	 * @return Other negative errors: see the file description.
	 */
	int fdt_get_prop(struct fdt_node node, const char* name, struct fdt_prop* out);

	/**
	 * @brief \#address-cells of @p node: the number of cells in its *children's* addresses.
	 *
	 * Defaults to 2 if the property is absent (spec 2.3.5).
	 *
	 * @param node Node that holds the property (the parent of the nodes whose @c reg is decoded).
	 * @param out  The cell count.
	 * @retval SUCCESS        @p out is the count
	 * @retval -ERROR_GENERIC The property isn't a single u32
	 * @return Other negative errors: see the file description.
	 */
	int fdt_get_address_cells(struct fdt_node node, uint32_t* out);

	/**
	 * @brief \#size-cells of @p node: the number of cells in its *children's* sizes.
	 *
	 * Defaults to 1 if the property is absent (spec 2.3.5).
	 *
	 * @param node Node that holds the property (the parent of the nodes whose @c reg is decoded).
	 * @param out  The cell count.
	 * @retval SUCCESS        @p out is the count
	 * @retval -ERROR_GENERIC The property isn't a single u32
	 * @return Other negative errors: see the file description.
	 */
	int fdt_get_size_cells(struct fdt_node node, uint32_t* out);

	/**
	 * @brief Decodes entry @p idx of @p node's @c reg property (spec 2.3.6).
	 *
	 * The cell counts come from @p parent. The address is in @p parent's
	 * address space: no @c ranges translation is done.
	 *
	 * @param node   Node whose @c reg to read.
	 * @param parent @p node's parent.
	 * @param idx    Index of the (address, size) tuple, from 0.
	 * @param addr   Decoded address.
	 * @param size   Decoded size in bytes (0 if @p parent has \#size-cells = 0).
	 * @retval SUCCESS          @p addr and @p size hold tuple @p idx
	 * @retval -ERROR_NOT_FOUND No @c reg, or @p idx is past its last tuple
	 * @retval -ERROR_GENERIC   \#address-cells is 0 or above 2, \#size-cells is above 2
	 *                          (doesn't fit in 64 bits), or @c reg isn't a whole number of tuples
	 * @return Other negative errors: see the file description.
	 */
	int fdt_get_reg(struct fdt_node node, struct fdt_node parent, uint32_t idx, uint64_t* addr, uint64_t* size);

	/**
	 * @brief Translates an address on @p bus into @p bus_parent's address space (spec 2.3.8).
	 *
	 * Uses @p bus's @c ranges: (child address, parent address, size) entries, with
	 * the child address in @p bus's \#address-cells, the parent address in
	 * @p bus_parent's \#address-cells and the size in @p bus's \#size-cells. An empty
	 * @c ranges means the two address spaces are the same.
	 *
	 * Translates one level only. Example on the BCM2711: /soc maps the VideoCore
	 * bus address 0x7e000000 to the CPU address 0xfe000000 (low peripheral mode).
	 *
	 * @param bus        Node whose children use @p child_addr's address space (e.g. /soc).
	 * @param bus_parent @p bus's parent.
	 * @param child_addr Address as written in a child's @c reg.
	 * @param out        The address in @p bus_parent's space.
	 * @retval SUCCESS          @p out holds the translated address
	 * @retval -ERROR_NOT_FOUND @p bus has no @c ranges (not translatable), or no entry covers @p child_addr
	 * @retval -ERROR_GENERIC   A cell count is 0 or above 2 (e.g. PCI's 3-cell addresses),
	 *                          or @c ranges isn't a whole number of entries
	 * @return Other negative errors: see the file description.
	 */
	int fdt_translate_address(struct fdt_node bus, struct fdt_node bus_parent, uint64_t child_addr, uint64_t* out);

	/**
	 * @brief Reads one big-endian 32-bit cell.
	 * @param p Address of the cell (any alignment).
	 * @return The value in native endianness.
	 */
	uint32_t fdt_read_u32(const void* p);

	/**
	 * @brief Reads @p cell_count big-endian 32-bit cells as one number, most significant first.
	 * @param p          Address of the first cell (any alignment).
	 * @param cell_count Number of cells, 0 to 2 (0 gives 0).
	 * @return The value in native endianness.
	 */
	uint64_t fdt_read_cells(const void* p, uint32_t cell_count);

#ifdef __cplusplus
}
#endif
