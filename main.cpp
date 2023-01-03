#include <fstream>
#include <iostream>

int32_t registers[32] = {0};

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
 */
void REGISTERS(bool regWrite, uint8_t readReg1, uint8_t readReg2, uint8_t writeReg, const int32_t writeData, 
               int32_t& readData1, int32_t& readData2){
    // ensure that only 5 bits get used for reg addresses
    readReg1 &= 0b11111;
    readReg2 &= 0b11111;
    writeReg &= 0b11111;

    // write data but dicard changes to the $zero/$ra register (hardwired)
    if(regWrite && writeReg != 0u && writeReg != 31u){
        registers[writeReg] = writeData;
    }

    // read data from registers
    readData1 = registers[readReg1];
    readData2 = registers[readReg2];
}

void printAllRegisters(){
    for(int i = 0; i < 32; ++i){
        std::cout << i << ": " << registers[i] << std::endl;
    }
    std::cout << "-------" << std::endl;
}

void ALU(u_int64_t input1, u_int64_t input2, u_int8_t ALUControl, u_int64_t &resultALU, u_int64_t &zeroFlag) {
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

void ALUControl(u_int8_t ALUOp, u_int8_t funct, u_int8_t &ALUControl) {
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

int main(int argc, char* argv[]) {
    std::cout << "Hello World";
    u_int64_t resultALU, zeroFlag;
    ALU(1,2,2,resultALU,zeroFlag);
    std::cout << resultALU;
    
    int32_t a,b;
    printAllRegisters();
    REGISTERS(1, 0, 0, 1, -42, a, b);
    printAllRegisters();
    REGISTERS(0, 1, 2, 0, 0, a, b);
    std::cout << a << "; " << b << std::endl;
}