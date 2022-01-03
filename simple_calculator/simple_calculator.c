/*
	Making a simple calculator that can perform following operations.
	- Bianary arithmetic operations(+, -, *, /)
	- Move some value to register
	- Storing value to some register
	- Compare two values or registers
	- Jump to specific instruction line
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sum(char *opr1, char *opr2);
void sub(char *opr1, char *opr2);
void multiply(char *opr1, char *opr2);
void divide(char *opr1, char *opr2);
void move(char *opr1, char *opr2);
void jump(char *opr1, FILE* input, char buffer[]);
void compare(char *opr1, char *opr2);
void branch(char *opr1, FILE* input, char buffer[]);
int opr_dist(char *opr);
int R[10];  // 전역변수로 레지스터 10개선언

int main(int argc, char* argv[])
{
        char buffer[100];
        char *opc;
        char *opr1;
        char *opr2;
        FILE* input;
        if(argc == 2)
        {
                input = fopen(argv[1], "r");
        }
        else
        {
                input = fopen("input.txt", "r");
        }
        while(fgets(buffer, 100, input))
        {
                if(buffer[strlen(buffer)-1] == '\n')
                {
                        buffer[strlen(buffer) - 1] = '\0';
                }
                printf("%-10s --> ", buffer);
                opc = strtok(buffer, " ");
                opr1 = strtok(NULL, " ");
                opr2 = strtok(NULL, " ");
                switch(*opc)
                {
                case '+' :
                        sum(opr1, opr2);
                        break;
                case '-' :
                        sub(opr1, opr2);
                        break;
                case '*' :
                        multiply(opr1, opr2);
                        break;
                case '/' :
                        divide(opr1, opr2);
                        break;
                case 'M' :
                        move(opr1, opr2);
                        break;
                case 'J' :
                        jump(opr1, input, buffer);
                        break;
                case 'C' :
                        compare(opr1, opr2);
                        break;
                case 'B' :
                        branch(opr1, input, buffer);
                        break;
                case 'H' :
                        printf("계산 종료\n");
                        return 0;
                default :
                        printf("연산자 오류\n");
                }
        }
        fclose(input);
        return 0;
}

int opr_dist(char *opr)     // Operand가 레지스터인지 16진수인지 구분
{
        int num;
        if(*opr == 'R')
        {
                num = R[atoi(opr+1)];
        }
        else
        {
                num = strtol(opr, NULL, 16);
        }
        return num;
}

void sum(char *opr1, char *opr2)     // 더하기
{
        int opr1_dist = opr_dist(opr1);
        int opr2_dist = opr_dist(opr2);
        printf("R0: %d = %d + %d\n", opr1_dist + opr2_dist, opr1_dist, opr2_dist);
        R[0] = opr1_dist + opr2_dist;
}

void sub(char *opr1, char *opr2)     // 빼기
{
        int opr1_dist = opr_dist(opr1);
        int opr2_dist = opr_dist(opr2);
        printf("R0: %d = %d - %d\n", opr1_dist - opr2_dist, opr1_dist, opr2_dist);
        R[0] = opr1_dist - opr2_dist;
}

void multiply(char *opr1, char *opr2)     // 곱하기
{
        int opr1_dist = opr_dist(opr1);
        int opr2_dist = opr_dist(opr2);
        printf("R0: %d = %d * %d\n", opr1_dist * opr2_dist, opr1_dist, opr2_dist);
        R[0] = opr1_dist * opr2_dist;
}

void divide(char *opr1, char *opr2)     // 나누기
{
        int opr1_dist = opr_dist(opr1);
        int opr2_dist = opr_dist(opr2);
        if(opr2_dist == 0)
        {
                printf("0으로 나눌 수 없습니다.\n");
        }
        else
        {
                printf("R0: %d = %d / %d\n", opr1_dist / opr2_dist, opr1_dist, opr2_dist);
                R[0] = opr1_dist / opr2_dist;
        }
}

void move(char *opr1, char *opr2)      // 무브
{
        int opr2_dist = opr_dist(opr2);
        R[atoi(opr1+1)] = opr2_dist;
        printf("%s: %d\n", opr1, opr2_dist);
}

void jump(char *opr1, FILE* input, char buffer[])   // 점프
{
        int i;
        int line;
        line = strtol(opr1, NULL, 16);
        printf("Jump to %dth instruction\n", line);
        rewind(input);
        for(i=0; i<line-1 ; i++)
        {
                fgets(buffer, 100, input);
        }
}

void compare(char *opr1, char *opr2)    // 대소 비교
{
        int opr1_dist = opr_dist(opr1);
        int opr2_dist = opr_dist(opr2);
        if(opr1_dist >= opr2_dist)
                {
                        R[0] = 0;
                        printf("R0:= 0\n");
                }
                else
                {
                        R[0] = 1;
                        printf("R0:= 1\n");
                }
}

void branch(char *opr1, FILE* input, char buffer[])    // 조건부 점프
{
        int i;
        int line;
        if(R[0] == 1)
        {
                line = strtol(opr1, NULL, 16);
                printf("Jump to %dth instruction. ∵R0:= 1 \n", line);
                rewind(input);
                for(i=0; i<line-1 ; i++)
                {
                        fgets(buffer, 100, input);
                }
        }
        else
        {
                printf("Don't Jump. ∵R0:= 0\n");
        }
}
