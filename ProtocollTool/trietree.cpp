#include "trietree.h"
#include "utils.h"

#include <boost/algorithm/string.hpp>
#include <regex>

TrieTree::TrieTree()
{
	this->root = getNode();
}
TrieTree::TrieTree(const std::initializer_list<std::string> entries) {
	this->root = getNode();

	for (const std::string& entry : entries) {
		this->insert(entry);
	}
}


void TrieTree::insert(const std::string key)
{
	struct TrieNode* pCrawl = this->root;

	
	// remove trailing spaces and convert to lower case
	std::string input = key;
	input = trim(input);
	boost::to_lower(input);

	/*
	// check if input string is valid
	std::regex alpha_num{ "[a-z0-9]+" };
	if (!std::regex_match(input, alpha_num)) {
		throw IOException();
	}
	*/
	for (int level = 0; level < input.length(); level++) {
		int index = char_to_index(input[level]);
		if (!pCrawl->children[index])
			pCrawl->children[index] = getNode();

		pCrawl = pCrawl->children[index];
	}

	// mark last node as leaf
	pCrawl->isWordEnd = true;
}

const bool TrieTree::isLastNode(TrieNode* node)
{
	for (int i = 0; i < ALPHABET_SIZE; i++)
		if (node->children[i])
			return false;
	return true;
}

const void TrieTree::suggestionsRec(TrieNode* node, std::string currPrefix, int depth, std::string& suggestion)
{
	// found a string in Trie with the given prefix and it was not in the first layer of recursion
	if (node->isWordEnd && depth > 0) {
		//suggestion = currPrefix;
		return;
	}
	
	//suggestion = currPrefix;

	int child = -1;
	for (int i = 0; i < ALPHABET_SIZE; i++)
	{
		if (node->children[i] && child == -1) { // first child
			child = i;
		}
		else if (node->children[i] && child >= 0) { // second child
			return;
		}
	}

	if (child != -1) {
		suggestion.push_back(index_to_char(child));
		suggestionsRec(node->children[child],currPrefix + index_to_char(child), ++depth, suggestion);
	}
	
}

int TrieTree::char_to_index(const char& c)
{
	if (c >= 'A') {
		return (int)c - 26;
	}
	return (int)c;
}
char TrieTree::index_to_char(const int& i)
{
	if (i >= 'A') {
		return (char)(i + 26);
	}
	return (char)i;
}

const int TrieTree::findAutoSuggestion(const std::string query, std::string& suggestion)
{
	struct TrieNode* pCrawl = this->root;
	for (const char& c : query) {
		int ind = char_to_index(c);

		// no string in the Trie has this prefix
		if (!pCrawl->children[ind])
		{
			suggestion = "";
			return 0;
		}
			

		pCrawl = pCrawl->children[ind];
	}
	// If prefix is present as a word, but
	// there is no subtree below the last
	// matching node.
	if (isLastNode(pCrawl)) {
		suggestion = query;
		return -1;
	}

	suggestion = "";
	suggestionsRec(pCrawl, query, 0, suggestion);
	return 1;
}

TrieNode* TrieTree::getNode(void)
{
	struct TrieNode* pNode = new TrieNode;
	pNode->isWordEnd = false;

	for (int i = 0; i < ALPHABET_SIZE; i++)
		pNode->children[i] = NULL;

	return pNode;
}
void TrieTree::deleteFollowing(TrieNode* node)
{
	for (auto i = 0; i < ALPHABET_SIZE; i++) {
		if (node->children[i]) {
			deleteFollowing(node->children[i]);
		}
	}

	delete node;
}
