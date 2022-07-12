#pragma once

#include <string>
#include <list>
#define ALPHABET_SIZE (256)
#define char_to_index(c) ((int) c)


struct TrieNode {
	struct TrieNode* children[ALPHABET_SIZE];
	bool isWordEnd;
};

class TrieTree {
public:
	TrieTree();
	TrieTree(const std::initializer_list<std::string> entries);
	template <typename Container>
	TrieTree(const Container entries);
	
	void insert(const std::string key);
	const bool isLastNode(struct TrieNode* node);
	const int findAutoSuggestions(const std::string query, std::string& suggestion);
private:
	static struct TrieNode* getNode(void);
	void deleteFollowing(struct TrieNode* node);
	const void suggestionsRec(struct TrieNode* node, std::string currPrefix, int depth, std::string& suggestion);
	struct TrieNode* root;
	//int char_to_index(const char& c);
};

template<typename Container>
inline TrieTree::TrieTree(const Container entries)
{
	this->root = getNode();
	for (const auto& entry : entries)
		this->insert(entry);
}
