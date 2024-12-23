#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int doubleword;

class Assembler
{
private:
	enum class Command
	{
		UNKNOWN = 0,
		MOV = 1,
		LOAD = 2,
		STORE = 3,
		ADD = 4,
		SUB = 5,
		JMP = 6,
		JZ = 7,
		PUSH = 8,
		POP = 9,
		HALT = 31,
	};
	struct Instruction
	{
		Command command;
		char operand_1[16];
		char operand_2[16];

		void set_operand_1(std::string _operand_1) { strncpy_s(operand_1, _operand_1.c_str(), _TRUNCATE); }
		void set_operand_2(std::string _operand_2) { strncpy_s(operand_2, _operand_2.c_str(), _TRUNCATE); }
	};
	struct Constant
	{
		char name[16];
		char type = 'b';
		char value[16];

		void set_name(std::string _name) { strncpy_s(name, _name.c_str(), _TRUNCATE); }
		void set_type(char _type) { type = _type; }
		void set_value(std::string _value) { strncpy_s(value, _value.c_str(), _TRUNCATE); }
	};

	Command parse_command(const std::string& commandStr)
	{
		static const std::unordered_map<std::string, Command> commandMap = {
			{"LOAD",  Command::LOAD},
			{"STORE", Command::STORE},
			{"ADD",   Command::ADD},
			{"SUB",   Command::SUB},
			{"JMP",   Command::JMP},
			{"JZ",    Command::JZ},
			{"HALT",  Command::HALT},
			{"PUSH",  Command::PUSH},
			{"POP",   Command::POP},
			{"MOV",   Command::MOV}
		};
		auto it = commandMap.find(commandStr);
		if (it != commandMap.end())
			return it->second;
		return Command::UNKNOWN;
	}

public:
	std::vector <Instruction> instructions;
	std::vector <Constant> constants;



	void parse_file(std::ifstream file)
	{
		byte flags = 0;
		for (std::string str = ""; std::getline(file, str); )
		{
			std::string command, arg_1, arg_2;
			std::stringstream ss(str);
			ss >> command;
			if (command == "section")
			{
				ss >> arg_1;
				if (arg_1 == ".data")
					flags = 1;
				if (arg_1 == ".text")
					flags = 2;
				continue;
			}
			if (flags == 1)
			{
				Constant constant;
				ss >> arg_1;
				for (std::string args; ss >> args; arg_2 += args) {}
				constant.set_name(command);
				constant.set_value(arg_2);
				if (arg_1.size() == 1 && !arg_2.empty()) constant.set_type(arg_1[0]);
				else { constant.set_type('b'); constant.set_value(arg_1); }
				constants.push_back(constant);
			}
			if (flags == 2)
			{
				Instruction instruction;
				instruction.command = parse_command(command);
				instruction.set_operand_1("");
				instruction.set_operand_2("");
				if (instruction.command == Command::MOV)
				{
					ss >> arg_1;
					instruction.set_operand_1(arg_1);
					ss >> arg_2;
					instruction.set_operand_2(arg_2);
				}
				else if (instruction.command == Command::HALT || instruction.command == Command::UNKNOWN)
					continue;
				else { ss >> arg_1; instruction.set_operand_1(arg_1); }
				instructions.push_back(instruction);
			}
		}
		file.close();
	}

	void assemble()
	{
		std::ofstream file("program.bin", std::ios::out | std::ios::binary);
		byte size = constants.size();
		file.write((char*)&size, sizeof(byte));
		for (auto constant : constants)
			file.write((char*)&constant, sizeof(Constant));
		for (auto instruction : instructions)
			file.write((char*)&instruction, sizeof(Instruction));
	}
};

std::ifstream arg_handle(int argc, char* argv[])
{
	std::ifstream file;
	if (argc == 1)
	{
		std::string filename;
		std::cout << "Enter .asm file:\n";
		std::cin >> filename;
		file.open(filename);
	}
	else
	{
		if (std::string(argv[1]).size() < 4 || std::string(argv[1]).substr(std::string(argv[1]).rfind(".")) != ".asm")
			throw std::runtime_error("Not a .asm file\n");

		file.open(argv[1]);
	}
	if (!file) throw std::runtime_error("No such file\n");
	return file;
}

int main(int argc, char* argv[])
{
	Assembler assembler = Assembler();
	assembler.parse_file(arg_handle(argc, argv));
	assembler.assemble();

	return 0;
}
