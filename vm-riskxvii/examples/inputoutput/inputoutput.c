inline int scan_char() {
    char result;
    int addr = 0x0812;
    
    asm volatile("lw %[res], 0(%[adr])": [res]"=r"(result): [adr]"r"(addr));

    return result;
}
void prints(char *str){
    while(*str) *((char *)0x0800) = *(str++);
}
int main(){
    char result[100];
    //get consecutive characters from 0x0812
    int i = 0;
    prints("Enter or pass in a string: \n");
    while(i < 100){
        result[i] += scan_char();
        if(result[i] == '\0' || result[i] == '\n'){
            break;
        }
        i++;
    }
    prints("You entered: \n");
    prints(result);
}