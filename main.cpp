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
    const uint32_t get() const {return program_counter;};
    void print() {
        std::cout <<"-------------ProgramCounter--------------" << std::endl;
        std::cout << "value PC"<<std::endl;
    }

  private:
    uint32_t program_counter = 0;
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

void DataMemory(int32_t address, int32_t writeData, bool memWrite, bool memRead, int32_t &readData) {
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
void menuInterface(bool finish){
    std::cout << "-----------------------------" << std::endl;
    std::cout << "R. Register values M. Memory values (add h to display in hexadecimal)" << std::endl;
    std::cout << "PC. Program counter value ";
    if(!finish){
        std::cout << "F. Fields of the current instruction" << std::endl;
        std::cout << "S. Step Run. To run the entire program ";
    }
    std::cout << "Reset. To reset the program" << std::endl;
    std::cout << "Q. Quit" << std::endl;
    std::cout << "-----------------------------" << std::endl;
}

void display_instructions(int * array,int current_index){
    std::cout << "------------Instruction------------" << std::endl;
    for(int i=0; i<= (sizeof(array)) ; i++){
        if(i==current_index){
            std::cout << "Curent : " << std::dec << array[i] << std::endl;
        }else{
            std::cout << std::dec << array[i] << std::endl;
        }
    }
    if(current_index >sizeof(array) ){
        std::cout << "Current : " << std::endl;
    }
    
}

void main_interface(Registers reg) {
    char buffer[10];
    int array[]= {1050,1050,1050,1050,11};
    int current_index=0;
    bool finish = false;
    std::cout << "---------------------------------" << std::endl;
    std::cout << "|           Processor           |" << std::endl;
    std::cout << "---------------------------------" << std::endl;
    display_instructions(array,current_index);
    menuInterface(finish);
    fflush( stdout );
    scanf( "%[^\n]", buffer );
    fgetc( stdin );



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
            ProgramCounter pc = ProgramCounter();
            pc.print();
        }else if((strcmp(buffer, "S") == 0 || strcmp(buffer, "s") == 0) && !finish ) {
           // step
           //CallFonctionForTheInstruction
           //fonction(array[current_index])
           std::cout << array[current_index] << std::endl;
           current_index = current_index + 1;

           if( current_index > (sizeof(array)/sizeof(array[0])) -1){
                finish=true;
            }

        }else if(strcmp(buffer, "Reset") == 0 || strcmp(buffer, "reset") == 0) {
           //reset
           //call reset fonction
           current_index = 0 ;
           finish=false;
        }else if((strcmp(buffer, "Run") == 0 || strcmp(buffer, "run") == 0) && !finish) {
            //run
            for(int i=0; i< (sizeof(array)/sizeof(array[0])) ; i++){
                //CallFonctionForTheInstruction
                //fonction(array[i])
                std::cout << "test "<< std::dec <<array[i] << std::endl;
            }
            current_index = (sizeof(array)/sizeof(array[0]));
            finish=true;
           
        }else if((strcmp(buffer, "f") == 0 || strcmp(buffer, "F") == 0) && !finish) {
            //Call format instruction 
            //fonction(array[current_index])
            std::cout << "Format " <<array[current_index] << std::endl;
        }else{
            std::cout << "Failed operation "<< std::endl;
        }

        display_instructions(array,current_index);
        menuInterface(finish);

        fflush( stdout );
        scanf( "%[^\n]", buffer );
        fgetc( stdin );
    }
}

void step(const int32_t& instruction, Registers& reg, Control& c, ProgramCounter& pc){
    // 1. Fetch instruction
    int8_t op_code = instruction >> 26;
    int8_t arg1 = (instruction >> 21) & 0b11111;
    int8_t arg2 = (instruction >> 16) & 0b11111;
    int8_t arg3 = (instruction >> 11) & 0b11111;
    int16_t arg4 = instruction & 65535; // first 16 bits
    uint32_t pc_next = pc.get() + 4; // PC + 4
    c.update(op_code); // Control Unit
    int32_t arg4_32 = int32_t(arg4); // sign extended

    // 2. registers
    int8_t write_reg = c.get(Control::REG_DST) ? arg3 : arg2; // mux
    int32_t read_data1, read_data2; // result
    reg.action(c.get(Control::REG_WRITE), arg1, arg2, write_reg, 0, read_data1, read_data2);

    // 3. ALU
    int32_t alu_input1 = read_data1;
    int32_t alu_input2 = c.get(Control::ALU_SRC) ? arg4_32 : read_data2; // MUX
    int8_t alu_op = c.get(Control::ALU_OP1) && c.get(Control::ALU_OP2);
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

int main(int argc, char* argv[]) {
    Control c;
    ProgramCounter pc;
    Registers reg;

    int32_t instruction =  0x1180fffb;
    step(instruction, reg, c, pc);
}
