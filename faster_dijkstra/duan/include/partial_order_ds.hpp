/**
 * Partial Order Data Structure (Lemma 3.1)
 *
 * A block-based data structure for adaptively partitioning problems into sub-problems.
 * Supports:
 * - Insert(key, value): O(max{1, log(N/M)}) amortized
 * - BatchPrepend(L): O(L·log(L/M)) amortized
 * - Pull(): Returns ≤M smallest keys in O(M) amortized
 *
 * Maintains two sequences of blocks:
 * - D₀: Elements from batch prepends (no size bound)
 * - D₁: Elements from inserts (O(N/M) blocks)
 */

#ifndef DUAN_PARTIAL_ORDER_DS_HPP
#define DUAN_PARTIAL_ORDER_DS_HPP

#include "common.hpp"
#include <list>
#include <map>
#include <unordered_map>
#include <utility>

namespace duan{

/**
 * KeyValuePair for storing elements
 */
using KeyValuePair = std::pair<int, long double>;  // (key, value)

/**
 * Block structure containing at most M key-value pairs
 */
struct Block {
    std::list<KeyValuePair> elements;  // Linked list of pairs
    long double upper_bound;           // Upper bound for this block

    Block() : upper_bound(INF) {}
    explicit Block(long double ub) : upper_bound(ub) {}

    int size() const { return elements.size(); }
    bool empty() const { return elements.empty(); }
};

/**
 * PartialOrderDS - Block-based partial ordering data structure
 *
 * Implementation of Lemma 3.1 from Duan et al. paper.
 *
 * Invariants:
 * - D₀ blocks are sorted by value (batch prepend only)
 * - D₁ blocks are sorted by value with BST of upper bounds
 * - Each D₁ block has Θ(M) elements (between M/4 and M after splits)
 * - Number of D₁ blocks is O(N/M)
 * - All values in earlier blocks ≤ values in later blocks
 */
class PartialOrderDS {
public:
    /**
     * Default constructor
     */
    PartialOrderDS() : M_(0), B_(INF), total_inserts_(0) {}

    /**
     * Initialize data structure
     * @param M Block size parameter
     * @param B Upper bound on all values
     */
    void Initialize(int M, long double B);

    /**
     * Insert a key-value pair
     * If key exists, update to minimum value
     * Time: O(max{1, log(N/M)}) amortized
     *
     * @param key Vertex identifier
     * @param value Distance value
     */
    void Insert(int key, long double value);

    /**
     * Batch prepend a list of key-value pairs
     * All values in L must be smaller than any existing value
     * If duplicate keys exist, keep smallest value
     * Time: O(L·log(L/M)) amortized
     *
     * @param L List of key-value pairs to prepend
     */
    void BatchPrepend(const vector<KeyValuePair>& L);

    /**
     * Pull smallest ≤M elements from data structure
     * Returns set S' of keys and separator value x
     * Time: O(M) amortized
     *
     * @return Pair of (keys vector, separator value)
     *         If DS becomes empty, separator = B
     *         Otherwise, max(S') < separator ≤ min(remaining)
     */
    std::pair<vector<int>, long double> Pull();

    /**
     * Check if data structure is empty
     * DS is empty when all blocks are empty
     */
    bool empty() const {
        for (const auto& block : D0_) {
            if (!block.empty()) return false;
        }
        for (const auto& block : D1_) {
            if (!block.empty()) return false;
        }
        return true;
    }

    /**
     * Get total number of elements (for debugging)
     */
    int total_elements() const;

private:
    // Two sequences of blocks
    std::list<Block> D0_;  // Batch prepend sequence
    std::list<Block> D1_;  // Insert sequence

    // Binary search tree for D1 block upper bounds
    // Maps upper_bound -> iterator to block in D1
    std::map<long double, std::list<Block>::iterator> D1_bounds_;

    // Track key locations for updates
    // Maps key -> (sequence_id, block_iterator, element_iterator)
    // sequence_id: 0 = D0, 1 = D1
    struct ElementLocation {
        int sequence_id;  // 0 or 1
        std::list<Block>::iterator block_it;
        std::list<KeyValuePair>::iterator elem_it;
    };
    std::unordered_map<int, ElementLocation> key_locations_;

    // Parameters
    int M_;              // Block size limit
    long double B_;      // Global upper bound
    int total_inserts_;  // Track inserts for analysis

    /**
     * Delete a key-value pair from its location
     * Time: O(1) for list deletion + O(log(N/M)) for potential block removal
     */
    void Delete(int key, const ElementLocation& loc);

    /**
     * Split a block in D1 that exceeds size M
     * Finds median, partitions into two blocks of size ≤⌈M/2⌉
     * Time: O(M) for median finding + O(log(N/M)) for BST update
     */
    void SplitBlock(std::list<Block>::iterator block_it);

    /**
     * Find median of elements in a block
     * Time: O(M) using selection algorithm
     */
    long double FindMedian(std::list<KeyValuePair>& elements);

    /**
     * Partition elements by median into two blocks
     * Elements < median go to first block, rest to second
     * Time: O(M)
     */
    std::pair<Block, Block> PartitionByMedian(
        std::list<KeyValuePair>& elements, long double median);

    /**
     * Create blocks from a list via recursive median finding
     * Each block will have at most ⌈M/2⌉ elements
     * Time: O(L·log(L/M))
     */
    std::list<Block> CreateBlocksFromList(vector<KeyValuePair>& L);

    /**
     * Find appropriate block in D1 for value
     * Returns iterator to block with smallest upper_bound ≥ value
     * Time: O(log(N/M))
     */
    std::list<Block>::iterator FindBlockForValue(long double value);

    /**
     * Update key location tracking
     */
    void UpdateKeyLocation(int key, int seq_id,
                          std::list<Block>::iterator block_it,
                          std::list<KeyValuePair>::iterator elem_it);

    /**
     * Remove key from location tracking
     */
    void RemoveKeyLocation(int key);

    /**
     * Collect prefix blocks from a sequence until total size ≥ M
     * Returns (collected_blocks, remaining_sequence_start)
     */
    std::pair<vector<KeyValuePair>, std::list<Block>::iterator>
    CollectPrefix(std::list<Block>& sequence, int target_size);
};

}

#endif
