#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string.h>

int32_t dataMemory[1000] = {0};
std::vector<uint32_t> instructionMemory;
bool finish = false;

class ProgramCounter {
   public:
    ProgramCounter() = default;
    void action(const uint32_t addrIn, uint32_t& addrOut) { addrOut = addrIn; program_counter = addrOut;}
    const uint32_t get() const {return program_counter;};
    void print() {
        std::cout <<"-------------ProgramCounter--------------" << std::endl;
        std::cout << program_counter <<std::endl;
    }

  private:
    uint32_t program_counter = 0;
};

struct Control {
   private:
    bool flags[9] = {0};

   public:
    bool get(const size_t& idx) { return flags[idx]; }
    enum LineNames { REG_DST, ALU_SRC, MEMTO_REG, REG_WRITE, MEM_READ, MEM_WRITE, BRANCH, ALU_OP1, ALU_OP2 };

    /**
     * @brief Update all control lines according to the provided op-code.
     *
     * @param input 6 bit op-code
     */
    void update(uint8_t input) {
        // ensure that only the first 6 bits are used
        input &= 0b111111;
        uint16_t output = 0b0;

        switch (input) {
            case 0b0:  // R-format
                output = 0b100100010;
                break;

            case 0b100011:  // lw
                output = 0b011110000;
                break;

            case 0b101011:  // sw
                output = 0b010001000;
                break;

            case 0b000100:  // beq
                output = 0b000000101;
                break;

            default:
                std::cout << "Error: op-code unknown." << std::endl;
                break;
        }

        // set flags depending on the output
        for (int i = 0; i < 9; ++i) {
            flags[i] = output & (0b1 << (8 - i));
        }
    }
};

/**
 * @brief ADD unit for the PC.
 *
 * @param addrIn 32 bit address coming from the PC AFTER increase it by 4
 * @param addrOut 32 bit address output
 * @param addrOffset 32 bit offset AFTER left shift by 2
 */
void PCAdd(uint32_t addrIn, uint32_t& addrOut, const uint32_t addrOffset) {
    addrOut = addrIn + addrOffset;
}

class Registers {
   private:
    int32_t registers[32] = {0};

   public:
    Registers() = default;

    /**
     * @brief Module for the registers ofc xD
     *
     * @param regWrite Control line for writing to registers. If set to 1, the data provided by writeData
     * will be stored in the register determined by writeReg.
     * @param readReg1 5 bit address of the first register to read
     * @param readReg2 5 bit address of the second register to read
     * @param writeReg 5 bit address of the register to write data to
     * @param writeData 32 bit data that should be stored in a register
     * @param readData1 32 bit data that was read from readReg1
     * @param readData2 32 bit data that was read from readReg2
     * @param hexa if you want to display in hexadecimal
     */
    void action(bool regWrite, uint8_t readReg1, uint8_t readReg2, uint8_t writeReg, const int32_t writeData,
                int32_t& readData1, int32_t& readData2) {
        // ensure that only 5 bits get used for reg addresses
        readReg1 &= 0b11111;
        readReg2 &= 0b11111;
        writeReg &= 0b11111;

        // write data but dicard changes to the $zero/$ra register (hardwired)
        if (regWrite && writeReg != 0u && writeReg != 31u) {
            registers[writeReg] = writeData;
        }

        // read data from registers
        readData1 = registers[readReg1];
        readData2 = registers[readReg2];
    }

    void reset() {
        for (int & i : registers){
            i = 0;
        }
    }

    void printAll(bool hexa) {
        std::cout << "--------------REGISTER------------------" << std::endl;
        for (int i = 0; i < 32; ++i) {
            if(registers[i] != 0){
                std::cout << "r" << i << " -> ";
                if(hexa){
                    std::cout <<  std::hex  ;
                }else{
                    std::cout << std::dec;
                }
                std::cout << registers[i] << " | ";
                if(i % 5 == 4){
                    std::cout << "" << std::endl;
                }
            }
        }
        std::cout << std::endl;
    }
};

void ALU(int32_t input1, int32_t input2, uint8_t ALUControl, int32_t &resultALU, bool &zeroFlag) {
    switch (ALUControl) {
        case 0b0000: // and
            resultALU = input1 & input2;
            break;
        case 0b0001: // or
            resultALU = input1 | input2;
            break;
        case 0b0010: // add
            resultALU = input1 + input2;
            break;
        case 0b0110: // sub
            resultALU = input1 - input2;
            break;
        case 0b0111: // slt
            resultALU = input1 < input2;
            break;
        case 0b1100: // nor
            resultALU = ~(input1 | input2);
            break;
    }
    resultALU == 0 ? zeroFlag = 1 : zeroFlag = 0;
}

void ALUControl(uint8_t ALUOp, uint8_t funct, uint8_t &ALUControl) {
    if (ALUOp == 0b10) {
        switch (funct) {
            case 0b0000: // add
                ALUControl = 0b0010;
                break;
            case 0b0010: // sub
                ALUControl = 0b0110;
                break;
            case 0b0100: // and
                ALUControl = 0b0000;
                break;
            case 0b0101: // or
                ALUControl = 0b0001;
                break;
            case 0b1010: // slt
                ALUControl = 0b0111;
                break;
        }
    } else if (ALUOp == 0b00) {
        ALUControl = 0b0010;
    } else if (ALUOp == 0b01) {
        ALUControl = 0b0110;
    }
}

void DataMemory(const int32_t address, int32_t writeData, bool memWrite, bool memRead, int32_t &readData) {
    if (memWrite == 1) {
        dataMemory[address] = writeData;
    } else if (memRead == 1) {
        readData = dataMemory[address];
    }
}

void printAllMemory(bool hexa) {
    std::cout << "--------------MEMORY------------------" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        if(dataMemory[i] != 0){
            std::cout << "m" << i << " -> ";
            if(hexa){
                std::cout <<  std::hex  ;
            }else{
                std::cout << std::dec;
            }
            std::cout << dataMemory[i] << " | ";
            if(i % 5 == 4){
                std::cout << "" << std::endl;
            }
        }
    }
    std::cout << std::endl;
}

void InstructionMemory(uint32_t address, uint32_t &instruction) {
    instruction = instructionMemory[address];
}

void Initialize() {
    std::fstream instructionFile;
    instructionFile.open("instructionMemory.txt", std::ios::in);
    std::string line;
    uint64_t instruction;
    if (instructionFile.is_open()) {
        while(getline(instructionFile, line)){
            std::stringstream ss;
            ss << std::hex << line;
            ss >> instruction;
            instructionMemory.push_back(instruction);
        }
    }
    instructionFile.close();
}

void reset(ProgramCounter &pc, Registers &reg) {
    uint32_t address;
    pc.action(0,address);
    reg.reset();
    for (int & i : dataMemory) {
        i = 0;
    }
    finish = false;
}

void step(const int32_t& instruction, Registers& reg, Control& c, ProgramCounter& pc){
    // 1. Fetch instruction
    uint8_t op_code = instruction >> 26;
    uint8_t arg1 = (instruction >> 21) & 0b11111;
    uint8_t arg2 = (instruction >> 16) & 0b11111;
    uint8_t arg3 = (instruction >> 11) & 0b11111;
    int16_t arg4 = instruction & 65535; // first 16 bits
    uint32_t pc_next = pc.get() + 4; // PC + 4
    c.update(op_code); // Control Unit
    int32_t arg4_32 = int32_t(arg4); // sign extended

    // 2. registers
    uint8_t write_reg = c.get(Control::REG_DST) ? arg3 : arg2; // mux
    int32_t read_data1, read_data2; // result
    reg.action(false, arg1, arg2, write_reg, 0, read_data1, read_data2);

    // 3. ALU
    int32_t alu_input1 = read_data1;
    int32_t alu_input2 = c.get(Control::ALU_SRC) ? arg4_32 : read_data2; // MUX
    int8_t alu_op = (c.get(Control::ALU_OP2) << 1)  | c.get(Control::ALU_OP1);
    uint8_t alu_control; // result
    int32_t alu_result; // result
    bool alu_zero; // result
    ALUControl(alu_op, arg4 & 0b111111, alu_control);
    ALU(alu_input1, alu_input2, alu_control, alu_result, alu_zero);

    // 4. Data access
    int32_t da_read_data; // result
    DataMemory(alu_result, read_data2, c.get(Control::MEM_WRITE), c.get(Control::MEM_READ), da_read_data);

    // 5. Write back
    int32_t write_data = c.get(Control::MEMTO_REG) ? da_read_data : alu_result; // MUX
    reg.action(c.get(Control::REG_WRITE), arg1, arg2, write_reg, write_data, read_data1, read_data2);

    // 6. PC
    uint32_t pc_add_result;
    PCAdd(pc_next, pc_add_result, arg4_32 << 2);
    pc_add_result = c.get(Control::BRANCH) && alu_zero ? pc_add_result : pc_next; // MUX
    pc.action(pc_add_result, pc_next);

}

void menuInterface(){
    std::cout << "-----------------------------" << std::endl;
    std::cout << "R. Register values (add h to display in hexadecimal)"<< std::endl;
    std::cout << "M. Memory values (add h to display in hexadecimal)" << std::endl;
    std::cout << "PC. Program counter value " << std::endl;
    std::cout << "F. Fields of the instructions" << std::endl;
    if(!finish){
        std::cout << "S. Step Run." << std::endl;
        std::cout << "Run. To run the entire program " << std::endl;
    }
    std::cout << "Reset. To reset the program" << std::endl;
    std::cout << "Q. Quit" << std::endl;
    std::cout << "-----------------------------" << std::endl;
}

void display_instructions(ProgramCounter pc){
    std::cout << "------------Instruction------------" << std::endl;
    for(int i=0; i< instructionMemory.size() ; i++){
        std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0');
        if((pc.get()/4)==i){
            std::cout << instructionMemory[i] << " <--" << std::endl;
        }else{
            std::cout << instructionMemory[i] << std::endl;
        }
    }
    if(pc.get()/4 > instructionMemory.size() ){
        std::cout << "<--" << std::endl;
    }

}

void display_format( ){
    std::cout << "------------FORMAT------------" << std::endl;
    for(unsigned int i : instructionMemory){
        uint32_t op_code = (i >> 26) & 0b111111;
        std::cout << "Op code : " << std::dec <<  op_code << std::endl;
        if (op_code == 0b000000) { //R-format
            uint32_t func = (i >> 6) & 0b111111;
            std::cout << "func : " << func << std::endl;
        } else if (op_code == 0b000010) { //J-format

        } else { // I-format

        }
        uint32_t arg1 = (i >> 21) & 0b11111;
        uint32_t arg2 = (i >> 16) & 0b11111;
        uint32_t arg3 = (i >> 11) & 0b11111;
        uint32_t arg4 = i & 65535; // first 16 bits
        std::cout << std::hex << i << std::endl;
    }
}

void main_interface(Registers& reg, ProgramCounter& pc, Control& c) {
    char buffer[10];
    std::cout << "---------------------------------" << std::endl;
    std::cout << "|           Processor           |" << std::endl;
    std::cout << "---------------------------------" << std::endl;
    display_instructions(pc);
    menuInterface();
    fflush( stdout );
    scanf( "%[^\n]", buffer );
    fgetc( stdin );

    /* reset & fonction display fomrat & gestion du finish */

    while (strcmp(buffer, "Q") != 0 && strcmp(buffer, "q") != 0  ){

        if(strcmp(buffer, "R") == 0 || strcmp(buffer, "r") == 0 ){
            reg.printAll(false);
        }else if(strcmp(buffer, "RH") == 0 || strcmp(buffer, "rh") == 0 ){
            reg.printAll(true);
        }else if(strcmp(buffer, "M") == 0 || strcmp(buffer, "m") == 0){
            printAllMemory(false);
        }else if(strcmp(buffer, "MH") == 0 || strcmp(buffer, "mh") == 0) {
           printAllMemory(true);
        }else if(strcmp(buffer, "PC") == 0 || strcmp(buffer, "pc") == 0) {
            pc.print();
        }else if((strcmp(buffer, "S") == 0 || strcmp(buffer, "s") == 0) && !finish ) {
           // step
            uint32_t instruction;
            InstructionMemory(pc.get() / 4, instruction);
            step(instruction, reg, c, pc);
        }else if(strcmp(buffer, "Reset") == 0 || strcmp(buffer, "reset") == 0) {
           //reset
           //call reset fonction
        }else if((strcmp(buffer, "Run") == 0 || strcmp(buffer, "run") == 0) && !finish) {
            while (!finish) {
                uint32_t instruction;
                InstructionMemory(pc.get()/ 4, instruction);
                std::cout << instruction << std::endl;
                step(instruction, reg, c, pc);
            }

        }else if((strcmp(buffer, "f") == 0 || strcmp(buffer, "F") == 0) && !finish) {
            display_format();
        }else{
            std::cout << "Failed operation "<< std::endl;
        }

        display_instructions(pc);
        menuInterface();

        fflush( stdout );
        scanf( "%[^\n]", buffer );
        fgetc( stdin );
    }
}

int main(int argc, char* argv[]) {
    Control c = Control();
    ProgramCounter pc = ProgramCounter();
    Registers reg = Registers();
    Initialize();
    main_interface(reg,pc,c);
}
