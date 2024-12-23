#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <stack>
#include <sstream>
#include <cstring>
#include <iomanip>

typedef unsigned char byte;            //b
typedef unsigned short word;           //w
typedef unsigned int doubleword;       //d
typedef unsigned long long quadroword; //q
typedef long double tenbytes;          //t

class Interpreter
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
    };

    struct Constant
    {
        char name[16];
        char type = 'b';
        char value[16];
    };

    std::unordered_map<std::string, int> registers;
    std::unordered_map<std::string, std::pair<int, int>> constants; // pair<address, size>
    std::vector<byte> memory;
    std::stack<int> stack;
    int pc = 0; // Program counter
    bool running = true;

    int get_value(const std::string& operand)
    {
        if (registers.find(operand) != registers.end()) // регистры
            return registers[operand];
        else if (constants.find(operand) != constants.end()) // константы
        {
            int address = constants[operand].first;
            int size = constants[operand].second;
            int value = 0;
            for (int i = 0; i < size; ++i)
                value |= memory[address + i] << (i * 8);
            return value;
        }
        else
            return std::stoi(operand);
    }

    void set_value(const std::string& operand, int value)
    {
        if (registers.find(operand) != registers.end()) // регистры
            registers[operand] = value;
        else if (constants.find(operand) != constants.end()) // константы
        {
            int address = constants[operand].first;
            int size = constants[operand].second;
            for (int i = 0; i < size; ++i)
                memory[address + i] = (value >> (i * 8)) & 0xFF;
        }
    }

    std::vector<Instruction> instructions;
    void execute_instruction(const Instruction& instruction)
    {
        std::string op1 = instruction.operand_1;
        std::string op2 = instruction.operand_2;

        switch (instruction.command)
        {
        case Command::MOV:
            set_value(op1, get_value(op2));
            break;
        case Command::LOAD:
            registers["AC"] = get_value(op1);
            break;
        case Command::STORE:
            set_value(op1, registers["AC"]);
            break;
        case Command::ADD:
            registers["AC"] += get_value(op1);
            break;
        case Command::SUB:
            registers["AC"] -= get_value(op1);
            break;
        case Command::JMP:
            pc = get_value(op1) - 1; // -1 because pc will be incremented after this
            break;
        case Command::JZ:
            if (registers["ZF"] == 0)
                pc = get_value(op1) - 1; // -1 because pc will be incremented after this
            break;
        case Command::PUSH:
            stack.push(get_value(op1));
            break;
        case Command::POP:
            set_value(op1, stack.top());
            stack.pop();
            break;
        case Command::HALT:
            running = false;
            break;
        default:
            break;
        }
    }

public:
    void load_program(std::ifstream file)
    {
        if (!file)
            throw std::runtime_error("No such file\n");

        byte size;
        file.read((char*)&size, sizeof(byte));
        for (byte i = 0; i < size; ++i)
        {
            Constant constant;
            file.read((char*)&constant, sizeof(Constant));
            int value = std::stoi(constant.value);
            int address = memory.size();
            int constant_size = (constant.type == 'b') ? 1 :
                (constant.type == 'w') ? 2 :
                (constant.type == 'd') ? 4 :
                (constant.type == 'q') ? 8 :
                (constant.type == 't') ? 10 : 0; // константа не записывается при ошибке типа
            for (int j = 0; j < constant_size; ++j)
                memory.push_back((value >> (j * 8)) & 0xFF);
            constants[constant.name] = std::make_pair(address, constant_size);
        }

        while (file)
        {
            Instruction instruction;
            file.read((char*)&instruction, sizeof(Instruction));
            instructions.push_back(instruction);
        }

        file.close();
    }

    void execute()
    {
        registers = {
            {"AC", 0},
            {"R0", 0},
            {"R1", 0},
            {"R2", 0},
            {"R3", 0},
            {"ZF", 0},
        };
        for (; running && pc < instructions.size(); ++pc)
            execute_instruction(instructions[pc]);
    }

    void print_report() const
    {
        std::cout << "Program Execution Report:\n";
        std::cout << "-------------------------\n";

        std::cout << "Registers:\n";
        for (const auto& reg : registers)
        {
            std::cout << reg.first << ": " << reg.second << "\n";
        }

        std::cout << "\nConstants:\n";
        for (const auto& constant : constants)
        {
            std::string name = constant.first;
            int address = constant.second.first;
            int size = constant.second.second;
            int value = 0;
            for (int i = 0; i < size; ++i)
                value |= memory[address + i] << (i * 8);
            std::cout << name << ": " << value << "\n";
        }

        std::cout << "\nMemory:\n";
        for (size_t i = 0; i < memory.size(); ++i)
        {
            if (i % 16 == 0)
                std::cout << std::hex << std::setw(4) << std::setfill('0') << i << ": ";
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)memory[i] << " ";
            if ((i + 1) % 16 == 0)
                std::cout << "\n";
        }
        std::cout << std::dec;

        std::cout << "\nStack:\n";

        for (std::stack<int> temp_stack = stack; !temp_stack.empty(); temp_stack.pop())
            std::cout << temp_stack.top() << "\n";
    }
};

std::ifstream arg_handle(int argc, char* argv[])
{
    std::ifstream file;
    if (argc == 1)
    {
        std::string filename;
        std::cout << "Enter .bin file:\n";
        std::cin >> filename;
        file.open(filename, std::ios::in | std::ios::binary);
    }
    else
    {
        if (std::string(argv[1]).size() < 4 || std::string(argv[1]).substr(std::string(argv[1]).rfind(".")) != ".bin")
            throw std::runtime_error("Not a .bin file\n");

        file.open(argv[1], std::ios::in | std::ios::binary);
    }
    if (!file) throw std::runtime_error("No such file\n");
    return file;
}

int main(int argc, char* argv[])
{
    Interpreter interpreter;
    try
    {
        interpreter.load_program(arg_handle(argc, argv));
        interpreter.execute();
        interpreter.print_report();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
