/**
 * Implementation of PartialOrderDS from Lemma 3.1
 */

#include "../include/partial_order_ds.hpp"
#include <algorithm>
#include <cassert>
#include <unordered_set>

namespace duan{

void PartialOrderDS::Initialize(int M, long double B) {
    M_ = M;
    B_ = B;
    total_inserts_ = 0;

    // Clear sequences
    D0_.clear();
    D1_.clear();
    D1_bounds_.clear();
    key_locations_.clear();

    // Initialize D1 with single empty block with upper bound B
    D1_.emplace_back(B);
    D1_bounds_[B] = D1_.begin();
}

void PartialOrderDS::Insert(int key, long double value) {
    // Check if key already exists
    auto loc_it = key_locations_.find(key);
    if (loc_it != key_locations_.end()) {
        // Key exists - check if new value is better
        const auto& loc = loc_it->second;
        long double old_value = loc.elem_it->second;

        if (value < old_value) {
            // Delete old entry and insert new one
            Delete(key, loc);
        } else {
            // New value is not better, skip insertion
            return;
        }
    }

    total_inserts_++;

    // Find appropriate block in D1 for this value
    auto block_it = FindBlockForValue(value);

    // Insert into block's linked list (O(1))
    block_it->elements.emplace_back(key, value);
    auto elem_it = std::prev(block_it->elements.end());

    // Update key location
    UpdateKeyLocation(key, 1, block_it, elem_it);

    // Check if block needs splitting
    if (block_it->elements.size() > (size_t)M_) {
        SplitBlock(block_it);
    }
}

void PartialOrderDS::BatchPrepend(const vector<KeyValuePair>& L) {
    if (L.empty()) return;

    // Remove duplicates and keep minimum value for each key
    std::unordered_map<int, long double> min_values;
    for (const auto& [key, value] : L) {
        auto it = min_values.find(key);
        if (it == min_values.end()) {
            min_values[key] = value;
        } else {
            it->second = std::min(it->second, value);
        }
    }

    // Convert back to vector
    vector<KeyValuePair> L_filtered;
    L_filtered.reserve(min_values.size());
    for (const auto& [key, value] : min_values) {
        // Also check against existing keys
        auto loc_it = key_locations_.find(key);
        if (loc_it != key_locations_.end()) {
            const auto& loc = loc_it->second;
            long double old_value = loc.elem_it->second;
            if (value < old_value) {
                Delete(key, loc);
                L_filtered.emplace_back(key, value);
            }
        } else {
            L_filtered.emplace_back(key, value);
        }
    }

    if (L_filtered.empty()) return;

    int L_size = L_filtered.size();

    if (L_size <= M_) {
        // Create single block
        Block new_block;
        new_block.elements.assign(L_filtered.begin(), L_filtered.end());

        // Add to front of D0
        D0_.push_front(new_block);
        auto block_it = D0_.begin();

        // Update key locations
        for (auto elem_it = block_it->elements.begin();
             elem_it != block_it->elements.end(); ++elem_it) {
            UpdateKeyLocation(elem_it->first, 0, block_it, elem_it);
        }
    } else {
        // Create O(L/M) blocks via median finding
        auto new_blocks = CreateBlocksFromList(L_filtered);

        // Prepend all blocks to D0
        for (auto& block : new_blocks) {
            D0_.push_front(block);
            auto block_it = D0_.begin();

            // Update key locations
            for (auto elem_it = block_it->elements.begin();
                 elem_it != block_it->elements.end(); ++elem_it) {
                UpdateKeyLocation(elem_it->first, 0, block_it, elem_it);
            }
        }
    }
}

std::pair<vector<int>, long double> PartialOrderDS::Pull() {
    vector<int> result_keys;
    long double separator = B_;

    // Collect prefix from D0 (up to M elements)
    auto [S0, D0_remaining] = CollectPrefix(D0_, M_);

    // Collect prefix from D1 (up to M elements)
    auto [S1, D1_remaining] = CollectPrefix(D1_, M_);

    // Combine S0 and S1
    vector<KeyValuePair> S_combined;
    S_combined.reserve(S0.size() + S1.size());
    S_combined.insert(S_combined.end(), S0.begin(), S0.end());
    S_combined.insert(S_combined.end(), S1.begin(), S1.end());

    // If total ≤ M, return all
    if (S_combined.size() <= (size_t)M_) {
        // All elements returned
        for (const auto& [key, value] : S_combined) {
            result_keys.push_back(key);
            RemoveKeyLocation(key);
        }

        // Remove collected blocks from D0 and D1
        D0_.erase(D0_.begin(), D0_remaining);
        D1_.erase(D1_.begin(), D1_remaining);

        // Update D1_bounds_
        for (auto it = D1_.begin(); it != D1_.end(); ++it) {
            D1_bounds_[it->upper_bound] = it;
        }

        // Separator is B if empty
        separator = empty() ? B_ : INF;  // Will compute below if not empty

        if (!empty()) {
            // Find minimum remaining value
            long double min_val = INF;
            if (!D0_.empty() && !D0_.front().elements.empty()) {
                min_val = std::min(min_val, D0_.front().elements.front().second);
            }
            if (!D1_.empty() && !D1_.front().elements.empty()) {
                min_val = std::min(min_val, D1_.front().elements.front().second);
            }
            separator = min_val;
        }

        return {result_keys, separator};
    }

    // Need to select exactly M smallest from S_combined
    // Sort S_combined and take first M
    std::sort(S_combined.begin(), S_combined.end(),
              [](const KeyValuePair& a, const KeyValuePair& b) {
                  return a.second < b.second;
              });

    // Take first M elements
    result_keys.reserve(M_);
    for (int i = 0; i < M_ && i < (int)S_combined.size(); ++i) {
        result_keys.push_back(S_combined[i].first);
    }

    // Remove selected elements from D0 and D1
    std::unordered_set<int> selected_keys(result_keys.begin(), result_keys.end());
    for (int key : selected_keys) {
        auto loc_it = key_locations_.find(key);
        if (loc_it != key_locations_.end()) {
            Delete(key, loc_it->second);
        }
    }

    // Compute separator (smallest remaining value)
    separator = INF;
    if (!D0_.empty() && !D0_.front().elements.empty()) {
        for (const auto& [key, value] : D0_.front().elements) {
            separator = std::min(separator, value);
        }
    }
    if (!D1_.empty() && !D1_.front().elements.empty()) {
        for (const auto& [key, value] : D1_.front().elements) {
            separator = std::min(separator, value);
        }
    }

    if (separator == INF) separator = B_;

    return {result_keys, separator};
}

int PartialOrderDS::total_elements() const {
    int count = 0;
    for (const auto& block : D0_) {
        count += block.size();
    }
    for (const auto& block : D1_) {
        count += block.size();
    }
    return count;
}

// Private methods

void PartialOrderDS::Delete(int key, const ElementLocation& loc) {
    auto& block = *loc.block_it;
    block.elements.erase(loc.elem_it);

    // If D1 block becomes empty, remove from BST
    if (loc.sequence_id == 1 && block.empty()) {
        D1_bounds_.erase(block.upper_bound);
    }

    RemoveKeyLocation(key);
}

void PartialOrderDS::SplitBlock(std::list<Block>::iterator block_it) {
    auto& block = *block_it;

    // Find median
    long double median = FindMedian(block.elements);

    // Partition into two blocks
    auto [block1, block2] = PartitionByMedian(block.elements, median);

    // Update upper bounds
    block1.upper_bound = median;
    block2.upper_bound = block.upper_bound;

    // Remove old bound from BST
    D1_bounds_.erase(block.upper_bound);

    // Replace current block with block1
    *block_it = block1;
    D1_bounds_[block1.upper_bound] = block_it;

    // Insert block2 after block1
    auto block2_it = D1_.insert(std::next(block_it), block2);
    D1_bounds_[block2.upper_bound] = block2_it;

    // Update key locations
    for (auto elem_it = block_it->elements.begin();
         elem_it != block_it->elements.end(); ++elem_it) {
        UpdateKeyLocation(elem_it->first, 1, block_it, elem_it);
    }
    for (auto elem_it = block2_it->elements.begin();
         elem_it != block2_it->elements.end(); ++elem_it) {
        UpdateKeyLocation(elem_it->first, 1, block2_it, elem_it);
    }
}

long double PartialOrderDS::FindMedian(std::list<KeyValuePair>& elements) {
    // Convert to vector for median finding
    vector<long double> values;
    values.reserve(elements.size());
    for (const auto& [key, value] : elements) {
        values.push_back(value);
    }

    // Find median using nth_element
    size_t mid = values.size() / 2;
    std::nth_element(values.begin(), values.begin() + mid, values.end());
    return values[mid];
}

std::pair<Block, Block> PartialOrderDS::PartitionByMedian(
    std::list<KeyValuePair>& elements, long double median) {

    Block block1, block2;

    for (const auto& pair : elements) {
        if (pair.second < median) {
            block1.elements.push_back(pair);
        } else {
            block2.elements.push_back(pair);
        }
    }

    return {block1, block2};
}

std::list<Block> PartialOrderDS::CreateBlocksFromList(vector<KeyValuePair>& L) {
    // Recursively partition L into blocks of size ≤ M/2
    std::list<Block> blocks;

    if (L.size() <= (size_t)(M_ / 2)) {
        // Base case: create single block
        Block block;
        block.elements.assign(L.begin(), L.end());
        blocks.push_back(block);
        return blocks;
    }

    // Find median and partition
    vector<long double> values;
    values.reserve(L.size());
    for (const auto& [key, value] : L) {
        values.push_back(value);
    }

    size_t mid = values.size() / 2;
    std::nth_element(values.begin(), values.begin() + mid, values.end());
    long double median = values[mid];

    // Partition L by median
    vector<KeyValuePair> L_left, L_right;
    for (const auto& pair : L) {
        if (pair.second < median) {
            L_left.push_back(pair);
        } else {
            L_right.push_back(pair);
        }
    }

    // Recursively create blocks
    auto blocks_left = CreateBlocksFromList(L_left);
    auto blocks_right = CreateBlocksFromList(L_right);

    blocks.splice(blocks.end(), blocks_left);
    blocks.splice(blocks.end(), blocks_right);

    return blocks;
}

std::list<Block>::iterator PartialOrderDS::FindBlockForValue(long double value) {
    // Binary search in D1_bounds_ for smallest upper_bound ≥ value
    auto it = D1_bounds_.lower_bound(value);

    if (it == D1_bounds_.end()) {
        // Value is larger than all bounds, use last block
        return std::prev(D1_.end());
    }

    return it->second;
}

void PartialOrderDS::UpdateKeyLocation(
    int key, int seq_id,
    std::list<Block>::iterator block_it,
    std::list<KeyValuePair>::iterator elem_it) {

    key_locations_[key] = {seq_id, block_it, elem_it};
}

void PartialOrderDS::RemoveKeyLocation(int key) {
    key_locations_.erase(key);
}

std::pair<vector<KeyValuePair>, std::list<Block>::iterator>
PartialOrderDS::CollectPrefix(std::list<Block>& sequence, int target_size) {

    vector<KeyValuePair> collected;
    auto it = sequence.begin();

    while (it != sequence.end() && (int)collected.size() < target_size) {
        for (const auto& pair : it->elements) {
            collected.push_back(pair);
            if ((int)collected.size() >= target_size) {
                break;
            }
        }
        ++it;
    }

    return {collected, it};
}

}
