#include <iostream>
#include <string>
#include <sstream>
#include <cctype>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <boost/multiprecision/cpp_int.hpp>

using BigInt = boost::multiprecision::cpp_int;

enum class CommandType {
	CREATE,
	INSERT,
	PRINT_TREE,
	CONTAINS,
	SEARCH,
	UNKNOWN
};

enum class TokenType {
	WORD,
	NUMBER,
	LEFT_PAREN,
	RIGHT_PAREN,
	COMMA,
	INVALID
};

CommandType getCommandType(const std::string& word) {
	if (word == "CREATE") return CommandType::CREATE;
	if (word == "INSERT") return CommandType::INSERT;
	if (word == "PRINT_TREE") return CommandType::PRINT_TREE;
	if (word == "CONTAINS") return CommandType::CONTAINS;
	if (word == "SEARCH") return CommandType::SEARCH;

	return CommandType::UNKNOWN;
}

struct Command {
	CommandType type = CommandType::UNKNOWN;

	std::string set_name;

	BigInt x = 0;
	BigInt y = 0;

	bool valid = false;
	std::string error;
};

struct Token {
	TokenType type;
	std::string value;
};

struct Point {
	BigInt x;
	BigInt y;
};

bool isBlank(const std::string& text) {
	return std::all_of(text.begin(), text.end(), [](unsigned char c) {
		return std::isspace(c);
	});
}

class Parser {
public: 
	Command parse(const std::string& input_command) {

		size_t semicolon = input_command.find(';');
		if (semicolon == std::string::npos) {
			Command command;
			command.error = "Очікується ';'";
			return command;
		}

		std::string command_text = input_command.substr(0, semicolon);
		std::istringstream clean_stream(command_text);

		std::string first_word;
		if (!(clean_stream >> first_word)) {
			Command command;
			command.type = CommandType::UNKNOWN;
			command.error = "Порожня команда";
			return command;
		}

		for (char& c : first_word) {
			c = std::toupper(static_cast<unsigned char>(c));
		}

		CommandType type = getCommandType(first_word);

		if (type == CommandType::UNKNOWN) {
			Command command;
			command.type = CommandType::UNKNOWN;
			command.error = "Невідома команда";
			return command;
		}

		switch (type)
		{
		case CommandType::CREATE:
			return parseCreate(clean_stream);
		case CommandType::INSERT:
			return parseInsert(clean_stream);
		case CommandType::PRINT_TREE:
			return parsePrintTree(clean_stream);
		case CommandType::CONTAINS:
			return parseContains(clean_stream);
		case CommandType::SEARCH:
			return parseSearch(clean_stream);
		default:
			Command command;
			return command;
		}
	}

private:
	Command parseCreate(std::istringstream& stream) {
		return parseSetNameCommand(stream, CommandType::CREATE);
	}

	Command parsePrintTree(std::istringstream& stream) {
		return parseSetNameCommand(stream, CommandType::PRINT_TREE);
	}

	Command parseInsert(std::istringstream& stream) {
		return parsePointCommand(stream, CommandType::INSERT);
	}

	Command parseContains(std::istringstream& stream) {
		return parsePointCommand(stream, CommandType::CONTAINS);
	}

	Command parseSearch(std::istringstream& stream) {
		return parseSetNameCommand(stream, CommandType::SEARCH);
	}

	Command parseSetNameCommand(std::istringstream& stream, CommandType type) {
		Command command;

		command.type = type;
		if (!(stream >> command.set_name)) {
			command.error = "Очікується ім'я набору";
			return command;
		}
		
		if (!(command.valid = isValidIdentifier(command.set_name))) {
			command.error = "Неправильне ім'я";
			return command;
		}
		else {
			std::string extra;
			if (stream >> extra) {
				command.error = "Несподіваний текст після імені набору";
				command.valid = false;
				return command;
			}
		}

		return command;
	}
	bool isValidIdentifier(const std::string& name) {
		if (name.empty()) return false;

		if (!std::isalpha(static_cast<unsigned char>(name[0]))) return false;

		for (size_t i = 1; i < name.length(); i++) {
			if (!std::isalnum(static_cast<unsigned char>(name[i])) && static_cast<unsigned char>(name[i]) != '_') return false;
		}

		return true;
	}
	struct TokenRule {
		TokenType type;
		std::string error;
	};

	Command parsePointCommand(std::istringstream& stream, CommandType type) {
		Command command;

		command.type = type;

		std::string rest;
		std::getline(stream, rest);

		std::vector<Token> tokens = tokenize(rest);

		TokenRule rules[]{
			{ TokenType::WORD, "Очікується коректне ім'я набору" },
			{ TokenType::LEFT_PAREN, "Очікується '('" },
			{ TokenType::NUMBER, "Очікується координата x" },
			{ TokenType::COMMA, "Очікується ','" },
			{ TokenType::NUMBER, "Очікується координати y" },
			{ TokenType::RIGHT_PAREN, "Очікується ')'" }
		};

		size_t count = std::min(tokens.size(), std::size(rules));
		for (size_t i = 0; i < count; i++) {
			if (tokens[i].type != rules[i].type) {
				command.error = rules[i].error;
				return command;
			}
		}

		if (tokens.size() < std::size(rules)) {
			command.error = rules[tokens.size()].error;
			return command;
		}
		else if (tokens.size() > std::size(rules)) {
			command.error = "Несподіваний текст після координат";
			return command;
		}

		command.set_name = tokens[0].value;

		// TODO: long long -> довільна розрядність
		command.x = BigInt(tokens[2].value);
		command.y = BigInt(tokens[4].value);

		command.valid = true;

		return command;
	}

	std::vector<Token> tokenize(const std::string& text) {
		std::vector<Token> tokens;

		size_t i = 0;
		while (i < text.length()) {
			if (std::isspace(static_cast<unsigned char>(text[i]))) {
				i++;
				continue;
			}
			if (text[i] == '(') {
				tokens.push_back({ TokenType::LEFT_PAREN, "(" });
				i++;
				continue;
			}
			if (text[i] == ',') {
				tokens.push_back({ TokenType::COMMA, "," });
				i++;
				continue;
			}
			if (text[i] == ')') {
				tokens.push_back({ TokenType::RIGHT_PAREN, ")" });
				i++;
				continue;
			}
			if (std::isalpha(static_cast<unsigned char>(text[i]))) {
				size_t start = i;
				while (i < text.length() && (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_')) {
					i++;
				}
				std::string word = text.substr(start, i - start);
				tokens.push_back({ TokenType::WORD, word });
				continue;
			}

			bool numberStart = std::isdigit(static_cast<unsigned char>(text[i])) || (text[i] == '-' && i + 1 < text.length() && std::isdigit(static_cast<unsigned char>(text[i + 1])));
			if (numberStart) {
				size_t start = i;
				if (text[i] == '-') {
					i++;
				}

				while (i < text.length() && std::isdigit(static_cast<unsigned char>(text[i]))) {
					i++;
				}
				std::string number = text.substr(start, i - start);
				tokens.push_back({ TokenType::NUMBER, number });
				continue;
			}

			else { tokens.push_back({ TokenType::INVALID, std::string(1, text[i]) });  i++; }
		}
		return tokens;
	}
};

class KDTree {
private:
	struct Node {
		Point point;

		Node* left = nullptr;
		Node* right = nullptr;
	};

	Node* root = nullptr;

	Node* insertRecursive(Node* node, Point point, int depth, bool& inserted) {
		if (node == nullptr) {
			inserted = true;
			return new Node{ point };
		}

		if (point.x == node->point.x && point.y == node->point.y) {
			inserted = false;
			return node;
		}

		// Calculate current dimension (cd)
		int cd = depth % 2;

		if (cd == 0) {
			if (point.x < node->point.x) {
				node->left = insertRecursive(node->left, point, depth + 1, inserted);
			}
			else {
				node->right = insertRecursive(node->right, point, depth + 1, inserted);
			}
		}
		else {
			if (point.y < node->point.y) {
				node->left = insertRecursive(node->left, point, depth + 1, inserted);
			}
			else {
				node->right = insertRecursive(node->right, point, depth + 1, inserted);
			}
		}
		return node;

	}

	bool searchRecursive(Node* node, Point point, int depth) {
		if (node == nullptr) return false;
		if (node->point.x == point.x && node->point.y == point.y) return true;

		int cd = depth % 2;

		if (cd == 0) {
			if (point.x < node->point.x) {
				return searchRecursive(node->left, point, depth + 1);
			}
			else {
				return searchRecursive(node->right, point, depth + 1);
			}
		}
		else {
			if (point.y < node->point.y) {
				return searchRecursive(node->left, point, depth + 1);
			}
			else {
				return searchRecursive(node->right, point, depth + 1);
			}
		}
	}

	void printRecursive(Node* node, int depth, std::string prefix, bool isLast) {
		if (node == nullptr) return;
		
		// Те, що стоїть перед гілкою
		std::cout << prefix;

		if (isLast) std::cout << "└── ";
		else std::cout << "├── ";


		char axis = (depth % 2 == 0) ? 'X' : 'Y';

		std::cout << "(" << node->point.x << ", " << node->point.y << ")" << "[" << axis << "]\n";

		// Префікс для дітей цього вузла
		std::string childPrefix = prefix;

		if (isLast) childPrefix += "└── ";
		else childPrefix += "│   ";
		
		// Якщо є обидві дитини:
		if (node->left != nullptr && node->right != nullptr) {
			printRecursive(node->left, depth + 1, childPrefix, false);
			printRecursive(node->right, depth + 1, childPrefix, true);	
		}
		
		// Якщо є одна дитина left:
		else if (node->left != nullptr){
			printRecursive(node->left, depth + 1, childPrefix, true);
		}
		else if (node->right != nullptr) {
			printRecursive(node->right, depth + 1, childPrefix, true);

		}
	}

	void collectPoints(Node* node, std::vector<Point>& result) {
		if (node == nullptr) return;

		result.push_back(node->point);

		collectPoints(node->left, result);
		collectPoints(node->right, result);
	}

	void freeTree(Node* node) {
		if (!node) return;
		freeTree(node->left);
		freeTree(node->right);
		delete node;
	}

public:
	~KDTree() {
		freeTree(root);
	}

	bool insert(Point point) {
		bool inserted = false;
		root = insertRecursive(root, point, 0, inserted);
		return inserted;
	}

	bool search(Point point) {
		return searchRecursive(root, point, 0);
	}

	void print() {
		if (root == nullptr) {
			std::cout << "Tree is empty\n";
			return;
		}

		std::cout << "(" << root->point.x << ", " << root->point.y << ")\n";

		if (root->left != nullptr && root->right != nullptr) {
			printRecursive(root->left, 1, "", false);
			printRecursive(root->right, 1, "", true);
		}
		else if (root->left != nullptr) {
			printRecursive(root->left, 1, "", true);
		}
		else if (root->right != nullptr) {
			printRecursive(root->right, 1, "", true);
		}
	}

	void collect(std::vector<Point>& result) {
		collectPoints(root, result);
	}
};

class SetManager {
private:
	std::unordered_map<std::string, KDTree> sets;


public:
	// CREATE
	bool createSet(const std::string& name) {
		if (sets.find(name) != sets.end()) {
			return false;
		}

		sets.emplace(name, KDTree{});

		return true;
	}
	// INSERT
	bool insertPoint(const std::string& name, const Point& point) {
		auto it = sets.find(name);
		if (it == sets.end()) {
			return false;
		}
		
		return it->second.insert(point);
	}
	// CONTAINS
	bool hasSet(const std::string& name) {
		return sets.find(name) != sets.end();
	}

	bool containsPoint(const std::string& name, const Point& point) {
		auto it = sets.find(name);
		if (it == sets.end()) {
			return false;
		}
		return it->second.search(point);
	}
	// PRINT_TREE
	bool printTree(const std::string& name) {
		auto it = sets.find(name);
		if (it == sets.end()) {
			return false;
		}

		std::cout << "KDTree:\n";
		it->second.print();
		return true;
	}
	
	// SEARCH
	bool searchTree(const std::string& name, std::vector<Point>& result) {
		auto it = sets.find(name);
		if (it == sets.end()) {
			return false;
		}

		it->second.collect(result);
		return true;
	}
};


int main() {
	Parser parser;
	SetManager manager;

	std::string input_command;
	std::string command_buffer;

	std::cout << "Введіть 'help' для перегляду доступних команд або 'exit' для виходу.\n";

	while (true) {
		if (command_buffer.empty()) {
			std::cout << "> ";
		}
		else {
			std::cout << "... ";
		}

		if (!std::getline(std::cin, input_command)) break;
			
		if (command_buffer.empty() && input_command == "exit") {
			std::cout << "Програму завершено.\n";
			break;
		}

		else if (command_buffer.empty() &&  input_command == "help") {
			std::cout << "Доступні команди\n";
			std::cout << "CREATE set_name; - створення нової множини з назвою set_name.\n";
			std::cout << "INSERT set_name (x, y); - додавання нової точки з координатами (x, y) до множини set_name.\n";
			std::cout << "PRINT_TREE set_name; - виведення на екран внутрішньої структури KD-дерева, побудованого для множини set_name.\n";
			std::cout << "CONTAINS set_name (x, y); - перевірка входження точки (x, y) до множини set_name. \n";
			std::cout << "SEARCH set_name; - вивести всі точки множини.\n";
				
			continue;
		}
			
		else {
			if (command_buffer.empty() && isBlank(input_command)) {
				continue;
			}

			command_buffer += ' ';
		}
			
		command_buffer += input_command;

		if (command_buffer.find(';') != std::string::npos) {

			Command command = parser.parse(command_buffer);
			command_buffer.clear();

			if (!command.valid) {
				std::cout << "Error: " << command.error << '\n';
				continue;
			}

			if (command.type == CommandType::CREATE) {
				if (manager.createSet(command.set_name)) {
						std::cout << "Set " << command.set_name << " has been created\n";
				}
				else {
					std::cout << "Error: set " << command.set_name << " already exist\n";
				}
			}

			if (command.type == CommandType::INSERT) {
				Point point;
				
				point.x = command.x;
				point.y = command.y;
				
				if (!manager.hasSet(command.set_name)) {
					std::cout << "Error: set " << command.set_name << " doesn't exist\n";
					continue;
				}
				
				if (manager.insertPoint(command.set_name, point)) {
					std::cout << "Point inserted\n";
				}
				else {
					std::cout << "Point already exists\n";
				}
			}

			if (command.type == CommandType::CONTAINS) {
				Point point;
				point.x = command.x;
				point.y = command.y;
			
				if (!manager.hasSet(command.set_name)) {
					std::cout << "Error: set " << command.set_name << " doesn't exist\n";
					continue;
				}

				if (manager.containsPoint(command.set_name, point)) {
					std::cout << "TRUE\n";
				}
				else {
					std::cout << "FALSE\n";
				}
			}

			if (command.type == CommandType::PRINT_TREE) {
				if (!manager.printTree(command.set_name)) {
					std::cout << "Error: the tree of " << command.set_name << " doesn't exist\n";
				}
			}

			if (command.type == CommandType::SEARCH) {
				std::vector<Point> result;

				if (!manager.searchTree(command.set_name, result)) {
					std::cout << "Error: the tree of " << command.set_name << " doesn't exist\n";
				}
				else {
					for (const Point& point : result) {
						std::cout << "(" << point.x << ", " << point.y << ") ";
					}
					std::cout << "\n";
				}
			}
		}
		else {
			continue;
		}
	}
	return 0;
}