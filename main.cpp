#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string.h>

int32_t dataMemory[1000] = {0};
class ProgramCounter {
   public:
    ProgramCounter() = default;
    void action(const uint32_t addrIn, uint32_t& addrOut) { addrOut = addrIn; }
    void print() {
        std::cout <<"-------------ProgramCounter--------------" << std::endl;
        std::cout << "value PC"<<std::endl;
        std::cout << "----------------------------------------" << std::endl;
    }
};

struct Control {
   private:
    bool flags[9] = {0};

   public:
    bool get(const size_t& idx) { return flags[idx]; }
    enum LineNames { REG_DST, BRANCH, MEM_READ, MEMTO_REG, ALU_OP1, ALU_OP2, MEM_WRITE, ALU_SRC, REG_WRITE };

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

            case 101011:  // sw
                output = 0b010001000;
                break;

            case 0b000100:  // R-format
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

    void printAll(bool hexa) {
        std::cout << "--------------REGISTER------------------" << std::endl;
        std::cout << hexa << std::endl;
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
        std::cout << "----------------------------------------" << std::endl;
    }
};

void ALU(uint64_t input1, uint64_t input2, uint8_t ALUControl, uint64_t &resultALU, uint64_t &zeroFlag) {
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

void DataMemory(uint64_t address, uint64_t writeData, uint8_t memWrite, uint8_t memRead, uint64_t &readData) {
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
    std::cout << "----------------------------------------" << std::endl;
}

void InstructionMemory(std::fstream &file, uint64_t address, uint64_t &instruction) {
    file.clear();
    file.seekg(0);
    std::string line;
    if (file.is_open()) {
        for (int i = 0; i < address; i++) {
            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        getline(file,line);
    }
    std::stringstream ss;
    ss << std::hex << line;
    ss >> instruction;
}
void menuInterface(){
     std::cout << "R. Register values M. Memory values (add h to display in hexadecimal)" << std::endl;
    std::cout << "PC. Program counter value" << std::endl;
    std::cout << "S. Step Run. To run the entire program Reset. To reset the program" << std::endl;
    std::cout << "Q. Quit" << std::endl;
}

void main_interface(Registers reg) {
    char buffer[10];
    std::cout << "---------------------------------" << std::endl;
    std::cout << "|           Processor           |" << std::endl;
    std::cout << "---------------------------------" << std::endl;
    menuInterface();
    fflush( stdout );
    scanf( "%[^\n]", buffer );
    fgetc( stdin );
    while (strcmp(buffer, "Q") != 0 && strcmp(buffer, "q") != 0  ){
        menuInterface();
        if(strcmp(buffer, "R") == 0 || strcmp(buffer, "r") == 0 ){
            reg.printAll(false);
        }else if(strcmp(buffer, "RH") == 0 || strcmp(buffer, "rh") == 0 ){
            reg.printAll(true);
        }else if(strcmp(buffer, "M") == 0 || strcmp(buffer, "m") == 0){
            printAllMemory(false);
        }else if(strcmp(buffer, "MH") == 0 || strcmp(buffer, "mh") == 0) {
           printAllMemory(true);
        }else if(strcmp(buffer, "PC") == 0 || strcmp(buffer, "pc") == 0) {
            ProgramCounter pc = ProgramCounter();
            pc.print();
        }else if(strcmp(buffer, "S") == 0 || strcmp(buffer, "s") == 0) {
           // step
        }else if(strcmp(buffer, "Reset") == 0 || strcmp(buffer, "reset") == 0) {
           //reset
        }else if(strcmp(buffer, "Run") == 0 || strcmp(buffer, "run") == 0) {
           //run
        }
        fflush( stdout );
        scanf( "%[^\n]", buffer );
        fgetc( stdin );
    }

    

}

int main(int argc, char* argv[]) {
    uint64_t resultALU, zeroFlag, readData, instruction;
    std::fstream instructionFile, dataFile;
    instructionFile.open("instructionMemory.txt", std::ios::in);
    dataFile.open("dataMemory.txt", std::ios::in | std::ios::out);
    ALU(1,2,2,resultALU,zeroFlag);
    std::cout << resultALU << std::endl;

    Registers reg = Registers();

    DataMemory(5,5,1,0,readData);
    DataMemory(6,15,1,0,readData);
    DataMemory(5,4,0,1,readData);
    //std::cout << std::hex << readData << std::endl;
    printAllMemory(false);
    printAllMemory(true);
    InstructionMemory(instructionFile,8,instruction);
    InstructionMemory(instructionFile,9,instruction);

    int32_t a,b;
    reg.printAll(false);
    reg.action(1, 0, 0, 1, -42, a, b);
    reg.printAll(false);
    reg.printAll(true);
    reg.action(0, 1, 2, 0, 0, a, b);
    instructionFile.close();

    Control c;
    std::cout << c.get(Control::REG_DST) << std::endl;
    c.update(0b0);
    std::cout << c.get(Control::REG_DST) << std::endl;

    //main_interface(reg);

}