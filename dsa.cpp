
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <unordered_set>
#include <queue>
#include <cctype>
namespace KeywordBackend {

    // Node structure for Separate Chaining collision resolution
    struct HashNode {
        std::string key;
        int value;
        HashNode* next;

        HashNode(const std::string& k, int v) : key(k), value(v), next(nullptr) {}
    };

    // Custom Hash Table implementation from scratch
    class KeywordHashTable {
    private:
        std::vector<HashNode*> buckets;
        size_t capacity;
        size_t size;
        const float MAX_LOAD_FACTOR = 0.75f;

        size_t hashFunction(const std::string& key) const {
            size_t hash = 5381;
            for (char c : key) {
                hash = ((hash << 5) + hash) + c; 
            }
            return hash % capacity;
        }

        void rehash() {
            size_t old_capacity = capacity;
            capacity *= 2;
            std::vector<HashNode*> new_buckets(capacity, nullptr);

            for (size_t i = 0; i < old_capacity; ++i) {
                HashNode* curr = buckets[i];
                while (curr != nullptr) {
                    HashNode* next_node = curr->next;
                    size_t new_index = hashFunction(curr->key);
                    curr->next = new_buckets[new_index];
                    new_buckets[new_index] = curr;
                    curr = next_node;
                }
            }
            buckets = std::move(new_buckets);
        }

    public:
        KeywordHashTable(size_t cap = 101) : capacity(cap), size(0) {
            buckets.resize(capacity, nullptr);
        }

        ~KeywordHashTable() {
            clear();
        }

        void clear() {
            for (size_t i = 0; i < capacity; ++i) {
                HashNode* curr = buckets[i];
                while (curr != nullptr) {
                    HashNode* prev = curr;
                    curr = curr->next;
                    delete prev;
                }
                buckets[i] = nullptr;
            }
            size = 0;
        }

        void insertOrIncrement(const std::string& key) {
            size_t index = hashFunction(key);
            HashNode* curr = buckets[index];

            while (curr != nullptr) {
                if (curr->key == key) {
                    curr->value++;
                    return;
                }
                curr = curr->next;
            }

            HashNode* newNode = new HashNode(key, 1);
            newNode->next = buckets[index];
            buckets[index] = newNode;
            size++;

            if (static_cast<float>(size) / capacity > MAX_LOAD_FACTOR) {
                rehash();
            }
        }

        int getFrequency(const std::string& key) const {
            size_t index = hashFunction(key);
            HashNode* curr = buckets[index];
            while (curr != nullptr) {
                if (curr->key == key) return curr->value;
                curr = curr->next;
            }
            return 0;
        }

        void getAllEntries(std::vector<std::pair<std::string, int>>& out_entries) const {
            out_entries.clear();
            out_entries.reserve(size);
            for (size_t i = 0; i < capacity; ++i) {
                HashNode* curr = buckets[i];
                while (curr != nullptr) {
                    out_entries.push_back({curr->key, curr->value});
                    curr = curr->next;
                }
            }
        }

        size_t getSize() const { return size; }
    };

    // Core Backend Engine Class
    class KeywordEngine {
    private:
        KeywordHashTable table;
        std::unordered_set<std::string> stopWords;

        std::string cleanWord(const std::string& raw) const {
            std::string cleaned = "";
            for (char c : raw) {
                if (std::isalnum(static_cast<unsigned char>(c))) {
                    cleaned += std::tolower(static_cast<unsigned char>(c));
                }
            }
            return cleaned;
        }

    public:
        KeywordEngine() {
            // Default built-in stop words
            stopWords = {
                "the", "is", "at", "which", "and", "a", "an", "in", "to", "of", 
                "for", "on", "with", "as", "by", "that", "this", "it", "from", "or"
            };
        }

        void addStopWord(const std::string& word) {
            stopWords.insert(cleanWord(word));
        }

        void processText(const std::string& text) {
            std::stringstream ss(text);
            std::string word;
            while (ss >> word) {
                std::string cleaned = cleanWord(word);
                if (!cleaned.empty() && stopWords.find(cleaned) == stopWords.end()) {
                    table.insertOrIncrement(cleaned);
                }
            }
        }

        bool loadFromFile(const std::string& filepath) {
            std::ifstream file(filepath);
            if (!file.is_open()) return false;

            std::string line;
            while (std::getline(file, line)) {
                processText(line);
            }
            file.close();
            return true;
        }

        int queryFrequency(const std::string& keyword) const {
            return table.getFrequency(cleanWord(keyword));
        }

        // Retrieves Top-K frequent words using a Min-Heap for O(N log K) optimization
        std::vector<std::pair<std::string, int>> getTopK(size_t k) const {
            std::vector<std::pair<std::string, int>> allEntries;
            table.getAllEntries(allEntries);

            auto cmp = [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                return a.second > b.second; // Min-heap based on frequency
            };

            std::priority_queue<
                std::pair<std::string, int>, 
                std::vector<std::pair<std::string, int>>, 
                decltype(cmp)
            > minHeap(cmp);

            for (const auto& entry : allEntries) {
                minHeap.push(entry);
                if (minHeap.size() > k) {
                    minHeap.pop();
                }
            }

            std::vector<std::pair<std::string, int>> result;
            while (!minHeap.empty()) {
                result.push_back(minHeap.top());
                minHeap.pop();
            }
            
            // Reverse to get descending order (highest frequency first)
            std::reverse(result.begin(), result.end());
            return result;
        }

        bool exportToCSV(const std::string& filepath) const {
            std::ofstream file(filepath);
            if (!file.is_open()) return false;

            std::vector<std::pair<std::string, int>> entries;
            table.getAllEntries(entries);

            file << "Keyword,Frequency\n";
            for (const auto& entry : entries) {
                file << entry.first << "," << entry.second << "\n";
            }
            file.close();
            return true;
        }

        size_t getTotalUniqueKeywords() const {
            return table.getSize();
        }

        void reset() {
            table.clear();
        }
    }
}