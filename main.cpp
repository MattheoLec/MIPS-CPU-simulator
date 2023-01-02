#include <fstream>
#include <iostream>

void ALU(u_int64_t input1, u_int64_t input2, u_int8_t ALUControl, u_int64_t &resultALU, u_int64_t &zeroFlag) {
    switch (ALUControl) {
        case 0: // and
            resultALU = input1 & input2;
            break;
        case 1: // or
            resultALU = input1 | input2;
            break;
        case 2: // add
            resultALU = input1 + input2;
            break;
        case 6: // sub
            resultALU = input1 - input2;
            break;
        case 7: // slt
            resultALU = input1 < input2;
            break;
        case 12: // nor
            resultALU = ~(input1 | input2);
            break;
    }
    resultALU == 0 ? zeroFlag = 1 : zeroFlag = 0;
}

int main(int argc, char* argv[]) {
    std::cout << "Hello World";
    u_int64_t resultALU, zeroFlag;
    ALU(1,2,2,resultALU,zeroFlag);
    std::cout << resultALU;
}