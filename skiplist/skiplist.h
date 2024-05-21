#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <fstream>

#define STORE_FILE "../store/dumpFile"

std::mutex mtx;
std::string delimiter = ":";

template<typename K, typename V>
class Node {
public:
	Node();
	Node(K k, V v, int);
	~Node();
	K get_key() const;
	V get_value() const;
	void set_value(V);
	Node<K, V>** forward; //指针数组，指向同一层的下一个节点，下标代表层数
	/*
	* leve1 1: 1 -> 3 -> 7，则节点1的forward[1]放的是节点3的地址，指向节点3；
	* 节点3的forward[1]放的是节点7的地址，指向节点7
	*/
	int node_level;
private:
	K key;
	V value;
};

template <typename K, typename V>
Node<K, V>::Node(const K k, const V v, int level) {
	this->key = k;
	this->value = v;
	this->node_level = level;
	
	this->forward = new Node<K, V>* [level + 1];
	memset(this->forward, 0, sizeof(Node<K, V>*) * (level + 1));
}

template <typename K, typename V>
Node<K, V>::~Node() {
	delete []forward;
}

template <typename K, typename V>
K Node<K, V>::get_key() const{
	return key;
}

template <typename K, typename V>
V Node<K, V>::get_value() const {
	return value;
}

template <typename K, typename V>
void Node<K, V>::set_value(V v) {
	this->value = v;
}

//跳表的模板类
template <typename K, typename V>
class SkipList {
public:
	SkipList() = default;
	SkipList(int);
	~SkipList();
	int get_random_level();
	int insert_element(K, V);
	void display_list();
	bool search_element(K);
	void delete_element(K);
	void update_element(K, V);
	void dump_file();
	void load_file();
	Node<K, V> *create_node(K, V, int);
	void clear(Node<K,V>*);
	int size();
private:
	void get_key_value_from_string(const std::string &str, std::string *key, std::string *value);
	bool is_valid_string(const std::string &str);
private:
	int _max_level;
	int _skip_list_level;
	Node<K, V>* _header;
	std::ofstream _file_writer;
	std::ifstream _file_reader;
	int _element_count;
};

template <typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K key, const V value, int level) {
	Node<K, V>* n = new Node<K,V>(key, value, level);
	return n;
}

template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value) {

	mtx.lock();
	Node<K, V>* current = this->_header; //current用来遍历的

	// update记录每一层要操作节点的前一个节点
	std::vector<Node<K, V>*> update(_max_level + 1, 0);
	/*Node<K, V>* update[_max_level + 1];
	memset(update, 0, sizeof(Node<K, V>*) * (_max_level + 1));*/

	// start form highest level of skip list 
	for (int i = _skip_list_level; i >= 0; i--) {
		while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {
			current = current->forward[i];
		}
		update[i] = current;
	}

	// reached level 0 and forward pointer to right node, which is desired to insert key.
	current = current->forward[0]; //for完以后肯定到了第0层，把current->forward[0]赋值给current，判断到没到第0层最后

	// if current node have key equal to searched key, we get it
	if (current != NULL && current->get_key() == key) {
		std::cout << "key: " << key << ", exists" << std::endl;
		mtx.unlock();
		return 1;
	}

	// if current is NULL that means we have reached to end of the level 
	// if current's key is not equal to key that means we have to insert node between update[0] and current node 
	if (current == NULL || current->get_key() != key) {

		// Generate a random level for node
		int random_level = get_random_level();

		// If random level is greater thar skip list's current level, initialize update value with pointer to header
		// 新节点的层比跳表层数大，只把高出跳表层的那几层初始化
		if (random_level > _skip_list_level) {
			for (int i = _skip_list_level + 1; i < random_level + 1; i++) {
				update[i] = _header;
			}
			_skip_list_level = random_level;
		}

		// create new node with random level generated 
		Node<K, V>* inserted_node = create_node(key, value, random_level);

		// insert node 
		// 每一层跟普通链表插入一样 insert->next=update->next; update->next=insert;
		// update就是记录的新节点的前一个节点
		for (int i = 0; i <= random_level; i++) {
			inserted_node->forward[i] = update[i]->forward[i];
			update[i]->forward[i] = inserted_node;
		}
		std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
		_element_count++;
	}
	mtx.unlock();
	return 0;
}

template <typename K, typename V>
void SkipList<K, V>::delete_element(K key) {
	mtx.lock();
	Node<K, V>* current = this->_header;
	std::vector<Node<K, V>*> update(_max_level + 1, 0);

	for (int i = _skip_list_level; i >= 0; i--) {
		while (current->forward[i] != nullptr && current->forward[i]->get_key() < key) {
			current = current->forward[i];
		}
		update[i] = current;
	}
	current = current->forward[0];

	if (current != nullptr && current->get_key()==key) {
		for (int i = 0; i <= _skip_list_level; i++) {
			if (update[i]->forward[i] != current) // 第i层没有要删除的节点
				break;
			update[i]->forward[i] = current->forward[i];
		}

		// 可能某一层只有一个节点，删除后就没有节点了，把这一层删除
		while (_skip_list_level > 0 && _header->forward[_skip_list_level] == nullptr) {
			_skip_list_level--;
		}

		std::cout << "Successfully deleted key " << key << std::endl;
		delete current;
		_element_count--;
	} else {
		std::cout << "没有要删除的目标节点" << std::endl;
	}
	mtx.unlock();
	return;
}

template <typename K, typename V>
bool SkipList<K, V>::search_element(K key) {
	std::cout << "\n-----------------search_element-----------------" << std::endl;
	Node<K, V>* current = _header;

	for (int i = _skip_list_level; i >= 0; i--) {
		while (current->forward[i]!=nullptr && current->forward[i]->get_key() < key) {
			current = current->forward[i];
		}
		if (current->forward[i] != nullptr && current->forward[i]->get_key() == key) {
			std::cout << "find key " << key << ":" << current->forward[i]->get_value() << " in level " << i << std::endl;
			return true;
		}
	}
	
	std::cout << key << " not find" << std::endl;
	return false;
}

template <typename K, typename V>
void SkipList<K, V>::update_element(const K key, const V value) {
	Node<K, V>* current = _header;

	for (int i = _skip_list_level; i >= 0; i--) {
		while (current->forward[i] != nullptr && current->forward[i]->get_key() < key) {
			current = current->forward[i];
		}
		if (current->forward[i] != nullptr && current->forward[i]->get_key() == key) {
			current->forward[i]->set_value(value);
			std::cout << "update successfully!" << std::endl;
			return;
		}
	}
}

template <typename K, typename V>
void SkipList<K, V>::display_list() {
	std::cout << "\n*****Skip List*****" << "\n";
	for (int i = _skip_list_level; i >= 0; i--) {
		Node<K, V>* node = this->_header->forward[i];
		std::cout << "Level " << i << ": ";
		while (node != nullptr) {
			std::cout << node->get_key() << ":" << node->get_value() << ";";
			node = node->forward[i];
		}
		std::cout << std::endl;
	}
}

template<typename K, typename V>
void SkipList<K, V>::dump_file() {
	std::cout << "dump file!" << std::endl;
	_file_writer.open(STORE_FILE);
	Node<K, V>* node = this->_header->forward[0];

	while (node != nullptr) {
		_file_writer << node->get_key() << ":" << node->get_value() << "\n";
		std::cout << node->get_key() << ":" << node->get_value() << "\n";
		node = node->forward[0];
	}
	_file_writer.flush();
	_file_writer.close();
	return;
}

template<typename K, typename V>
void SkipList<K, V>::load_file() {
	std::cout << "load file" << std::endl;
	_file_reader.open(STORE_FILE);
	std::string line;
	std::string* key = new std::string();
	std::string* value = new std::string();

	while (getline(_file_reader, line)) {
		get_key_value_from_string(line, key, value);
		if (key->empty() || value->empty()) {
			continue;
		}
		// Define key as int type
		insert_element(stoi(*key), *value);
		std::cout << "key:" << *key << " value:" << *value << std::endl;
	}
	delete key;
	delete value;
	_file_reader.close();
}

template<typename K, typename V>
int SkipList<K, V>::size() {
	return _element_count;
}

template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {

	if (!is_valid_string(str)) {
		return;
	}
	*key = str.substr(0, str.find(delimiter));
	*value = str.substr(str.find(delimiter) + 1, str.length());
}

template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {

	if (str.empty()) {
		return false;
	}
	if (str.find(delimiter) == std::string::npos) {
		return false;
	}
	return true;
}

template<typename K, typename V>
SkipList<K, V>::SkipList(int max_level) {

	this->_max_level = max_level;
	this->_skip_list_level = 0;
	this->_element_count = 0;

	// create header node and initialize key and value to null
	K k = K(); // 模板变量初始化
	V v = V();
	this->_header = new Node<K, V>(k, v, _max_level);
};

template<typename K, typename V>
SkipList<K, V>::~SkipList() {

	/*if (_file_writer.is_open()) {
		_file_writer.close();
	}
	if (_file_reader.is_open()) {
		_file_reader.close();
	}*/

	//递归删除跳表链条
	if (_header->forward[0] != nullptr) {
		clear(_header->forward[0]);
	}
	delete(_header);

}
template <typename K, typename V>
void SkipList<K, V>::clear(Node<K, V>* cur)
{
	if (cur->forward[0] != nullptr) {
		clear(cur->forward[0]);
	}
	delete(cur);
}

template<typename K, typename V>
int SkipList<K, V>::get_random_level() {

	int k = 1;
	while (rand() % 2) {
		k++;
	}
	k = (k < _max_level) ? k : _max_level;
	return k;
};