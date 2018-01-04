#include "comp.h"


const char CodeTable[8][4] = { "LIT", "LOD", "STO", "CAL", "INT", "JMP", "JPC", "OPR" };

int sym, num;
char id[maxIdLength + 1];
int pc = 1;
int line = 1;

void printError(const char *err)
{
    printf("Line %d:%s\n", line, err);
    exit(1);
}

//判断关键字种类
int findKeyword(const char *str)
{
    if (!strcmp(str, "const"))
        return CONSTSYM;
    if (!strcmp(str, "var"))
        return VARSYM;
    if (!strcmp(str, "procedure"))
        return PROCSYM;
    if (!strcmp(str, "begin"))
        return BEGINSYM;
    if (!strcmp(str, "end"))
        return ENDSYM;
    if (!strcmp(str, "odd"))
        return ODDSYM;
    if (!strcmp(str, "if"))
        return IFSYM;
    if (!strcmp(str, "then"))
        return THENSYM;
    if (!strcmp(str, "call"))
        return CALLSYM;
    if (!strcmp(str, "while"))
        return WHILESYM;
    if (!strcmp(str, "do"))
        return DOSYM;
    if (!strcmp(str, "write"))
        return WRITESYM;
    if (!strcmp(str, "read"))
        return READSYM;
    return -1;
}

int getSym(FILE *in)
{
    extern int sym, num;
    extern char id[maxIdLength + 1];
    char buf[maxIdLength + 1];
    int pos = 0;
    char ch = ' ';
    while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')//用于出去不必要的空格和换行等
    {
        if ((ch = fgetc(in)) == EOF) //测试代码全部读完
        {
            cout<<"\n::read finish::"<<endl;
            return -1;
        }
        if (ch == '\n')//程序行数加一
            line++;
    }
    if (isalpha(ch))//判断是否为字母
    {
        while (isalpha(ch) || isdigit(ch))//读取一个连续的词汇，如abc或者是关键字
        {
            if (pos >= maxIdLength)
                return -1;
            buf[pos++] = ch;
            ch = fgetc(in);
        }
        ungetc(ch, in);//回退到读入缓冲区
        buf[pos++] = '\0';
        sym = findKeyword(buf);

        if (sym<0)
        {
            sym = IDENT;//用户定义标识符
            //cout<<"IDENT:"<<buf<<endl;
            strcpy(id, buf);

            //判断是否已经存在ID表中
            string tempID=buf;
            bool flag=false;
            for(int i=0;i<idArrayCount;i++)
            {
                if(tempID.compare(IDArray[i])==0)
                {
                   flag=true;
                }
            }
            if(!flag)
            {
                IDArray[idArrayCount++]=buf;
            }
            return 0;
        }
        else
        {
            SYMArray[symArrayCount++]=buf;
        }
    }
    else if (isdigit(ch))//用户赋值数字
    {
        while (isdigit(ch))
        {
            if (pos >= maxIdLength)
                return -1;
            buf[pos++] = ch;
            ch = fgetc(in);
        }
        ungetc(ch, in);
        buf[pos++] = '\0';
        sym = NUM;
        num = atoi(buf);
        NUMArray[numArrayCount++]=buf;
        return 0;
    }
    else if (ch == '(')
        sym = LPAREN;
    else if (ch == ')')
        sym = RPAREN;
    else if (ch == '=')
        sym = EQ;
    else if (ch == '#')
        sym = NEQ;
    else if (ch == '+')
        sym = PLUS;
    else if (ch == '-')
        sym = MINUS;
    else if (ch == '*')
        sym = TIMES;
    else if (ch == '/')
        sym = SPLASH;
    else if (ch == ',')
        sym = COMMA;
    else if (ch == ';')
        sym = SEMICOLON;
    else if (ch == '.')
        sym = PERIOD;
    else if (ch == ':')
    {
        ch = fgetc(in);
        if (ch == '=')
            sym = ASSIGN;
        else
        {
            ungetc(ch, in);
            sym = COLON;
        }
    }
    else if (ch == '>')
    {
        ch = fgetc(in);
        if (ch == '=')
            sym = GE;
        else
        {
            ungetc(ch, in);
            sym = GT;
        }
    }
    else if (ch == '<')
    {
        ch = fgetc(in);
        if (ch == '=')
            sym = LE;
        else
        {
            ungetc(ch, in);
            sym = LSS;
        }
    }
    else return -1;

    return 0;
}

/**
*〈分程序〉→ [<常量说明部分>][<变量说明部分>][<过程说明部分>]〈语句〉
*/
int block(int level, int index)
{
    int count = 0, dx = 0;
    int tpc = pc;
    getTargetCode(JMP, 0, 0);//第一条为跳转指令，跳转地址未知
//    cout<<CodeTable[code[pc-1].op]<<"\t"<<code[pc-1].l<<"\t"<<code[pc-1].a<<endl;
    bool flag = false;
    //cout<<"current SYM:"<<sym<<endl;
    if (sym == CONSTSYM)
    {
//        cout<<"current SYM:CONSTSYM"<<endl;
        count = constDeclaration(index);
    }
    if (sym == VARSYM)
    {
//        cout<<"current SYM:VARSYM"<<endl;
        getSym();
        count += (dx = varDeclaration(level, index + count));
    }
    if (sym == PROCSYM)
    {
//        cout<<"current SYM:PROCSYM"<<endl;
        flag = true;//过程说明，将level+1
        getSym();
        int px = count + index;
        count += procDeclaration(level + 1, px);
    }
    if (!flag)
        pc--;//没有运行代码
    else
    {
        code[tpc].a = pc;//最后设置首条跳转的指令的地址。
//       cout<<CodeTable[code[tpc].op]<<"\t"<<code[tpc].l<<"\t"<<code[tpc].a<<endl;
    }
    getTargetCode(INT, 0, numLinkData + dx);
    statement(level, index + count);//调用语句部分
    getTargetCode(OPR, 0, OP_RET);
    return count;
}

/**
*<常量说明部分> → CONST<常量定义>{ ,<常量定义>}
*<常量定义> → <标识符>=<无符号整数>
*/
int constDeclaration(int index)
{
    Symbol sb;
    sb.type = CONST;
    int count = 0;
    do
    {
        getSym();
        if (sym != IDENT)
            printError("identifier expected! 未定义用户名");
        if (Myfind(0, index + count, id) >= 0)
            printError("duplicated identifier! 重复定义");
        strcpy(sb.name, id);
        getSym();
        if (sym != EQ)
            printError("a '=' expected! 缺少'='");
        getSym();
        if (sym != NUM)
            printError("number expected! 缺少初始化");
        sb.value = num;

        symTable[index + count++] = sb;//添加到symTable中。
        count_sym++;
        getSym();
        if (!(sym == COMMA || sym == SEMICOLON))
            printError("comma or semicolon expected! 缺少结束符号");
    }
    while (sym != SEMICOLON);  //如果是逗号则表示为多个定义。
    getSym();
    return count;
}

/**
* <变量说明部分> → VAR<标识符>{ ,<标识符>}；
* <标识符> → <字母>{<字母>|<数字>}
*/
int varDeclaration(int level, int index)
{
    Symbol sb;
    sb.type = VARIABLE;
    sb.level = level;
    sb.address = 0;
    int count = 0;
    int tsym = sym;
    do
    {
        if (sym != IDENT)
            printError("identifier expected! 未定义用户名");
        if (Myfind(0, index + count, id) >= 0)
            printError("duplicated expected! 重复定义");
        strcpy(sb.name, id);

        symTable[index + count++] = sb;
        count_sym++;
        sb.address++;
        getSym();
        if (!(sym == COMMA || sym == SEMICOLON))
            printError("comma or semicolon expected! 缺少结束符号");
        tsym = sym;
        getSym();
    }
    while (tsym != SEMICOLON);
    return count;
}

/**
*<过程说明部分> → <过程首部><分程度>；{<过程说明部分>}
*/
int procDeclaration(int level, int index)
{
    int count = 0;//记录level中定义的变量数
    if (sym != IDENT)
        printError("identifier expected! 未定义过程名");
    Symbol sb;
    strcpy(sb.name, id);
    sb.type = PROCEDURE;
    sb.level = level - 1;//?
    sb.address = pc;

    symTable[index + count++] = sb;
    count_sym++;
    getSym();
    if (sym != SEMICOLON)
        printError("semicolon expected! 缺少分号");
    getSym();
    block(level, index + count);
    if (sym != SEMICOLON)
        printError("semicolon expected! 缺少分号");
    getSym();
    if (sym == PROCSYM)//如果是procedure则再次调用procedureDeclaration
    {
        getSym();
        count += procDeclaration(level, index + count);
    }
    return count + 1;
}

/**
*<语句> → <赋值语句>|<条件语句>|<当型循环语句>|<过程调用语句>|<读语句>|<写语句>|<复合语句>|<空>
*/
int statement(int level, int index)
{
    if (sym == IFSYM)//if操作
    {
        getSym();
        ifStatement(level, index);
    }
    else if (sym == WHILESYM)//while操作
    {
        getSym();
        whileStatement(level, index);
    }
    else if (sym == CALLSYM)//call调用
    {
        getSym();
        callStatement(level, index);
    }
    else if (sym == WRITESYM)//写操作，即向屏幕输出
    {
        getSym();
        writeStatement(level, index);
    }
    else if (sym == READSYM)//从用户输入读取数字
    {
        getSym();
        readStatement(level, index);
    }
    else if (sym == BEGINSYM)//复杂语句，begin操作
    {
        getSym();
        compositeStatement(level, index);
    }
    else if (sym == IDENT)//<赋值语句>
    {
        assignStatement(level, index);
    }
    else
        return 0;
    return 0;
}

/**
*if条件判断<条件语句> → if<条件>then<语句>
*/
int ifStatement(int level, int index)
{
    condition(level, index);
    int cpc = pc;
    getTargetCode(JPC, 0, 0);
    if (sym != THENSYM)
        printError("then clause expected!");
    getSym();
    statement(level, index);
    code[cpc].a = pc;
    return 0;
}

/**
*while循环<当型循环语句> → while<条件>do<语句>
*/
int whileStatement(int level, int index)
{
    int cpc = pc;//记录当前while的初始位置
    condition(level, index);//调用条件处理
    int jpc = pc;
    getTargetCode(JPC, 0, 0);
    if (sym != DOSYM)
    {
        printError("do expected! 缺少标识符'do'");
    }
    getSym();
    statement(level, index);//继续调用statement <语句>
    getTargetCode(JMP, 0, cpc);//添加JMP指令，JMP地址为while的初始地址。
    code[jpc].a = pc;
    return 0;
}

/**
*<过程调用语句> → call<标识符>
*/
int callStatement(int level, int index)
{
    if (sym != IDENT)//判断是否为标识符
        printError("syntax error! 初始化错误");
    int i = Myfind(0, index, id);//找到相同名称的变量在symTable中的位置
    if (i<0)
        printError("identifier not found! 未定义用户名");
    if (symTable[i].type != PROCEDURE)
        printError("syntax error! 调用错误");


    //call调用设置genCode
    getTargetCode(CAL, level - symTable[i].level, symTable[i].address);//将symTable中的地址取出。
    getSym();
    return 0;
}

/**
*read方法调用 <读语句> → read(<标识符>{ ，<标识符>})
*/
int readStatement(int level, int index)
{
    if (sym != LPAREN)
        printError(" ( expected 缺少'('");
    getSym();
    while (sym != RPAREN)//当不为右括号的时候说明read未完
    {
        if (sym != IDENT)//不是用户自定义变量则出错
            printError("variable expected! 缺少自定义变量");
        int i = Myfind(0, index, id);//寻找变量是否已经定义过
        if (i<0)
            printError("identifier not found! 找不到对象");
        if (symTable[i].type != VARIABLE)//判断是否为常量
            printError("variable expected! 不是变量");
        getTargetCode(OPR, 0, OP_READ);//将读入的数据存入栈顶
        getTargetCode(STO, level - symTable[i].level, symTable[i].address + numLinkData);//将栈顶的内容送到某变量单元中
        getSym();
        if (sym != COMMA&&sym != RPAREN)//当后续的sym不为逗号或者是右括号的时候，出现错误
            printError("syntax error! Read缺失右括号或分号");
    }
    getSym();
    return 0;
}

/**
*<写语句> → write(<标识符>{，<标识符>})
*/
int writeStatement(int level, int index)
{
    if (sym != LPAREN)
        printError(" ( expected 缺少左括号。");
    getSym();
    while (sym != RPAREN)
    {
        expression(level, index);
        getTargetCode(OPR, 0, OP_WRITE);
        if (sym != COMMA&&sym != RPAREN)//当后续的sym不为逗号或者是右括号的时候，出现错误
            printError("syntax error! Read缺失右括号或分号");
    }
    getSym();
    return 0;
}

/**
*<复合语句> → begin<语句>{ ；<语句>}<end>
*/
int compositeStatement(int level, int index)
{
    statement(level, index);
    while (sym == SEMICOLON)
    {
        getSym();
        statement(level, index);//调用语句
    }
    if (sym != ENDSYM)
        printError("end expected!");
    getSym();
    return 0;
}

/**
*变量赋值语句<赋值语句> → <标识符>:=<表达式>
*/
int assignStatement(int level, int index)
{
    int i = Myfind(0, index, id);
    if (i<0)
    {
        printError("Variable not found! 未定义变量");
    }
    if (symTable[i].type == CONST)
        printError("constant can't be a r-value! 常量无法赋值");
    getSym();
    if (sym != ASSIGN)
    {
        printError(":= expected!");
    }
    getSym();
    expression(level, index);
    //将计算的结果存放到SymTable，其存放位置的层差为level - symTable[i].level，其所在层地址为numLinkData + symTable[i].address
    getTargetCode(STO, level - symTable[i].level, numLinkData + symTable[i].address);
    return 0;
}

/**
*<条件> → <表达式><关系运算符><表达式>|odd<表达式>
*/
int condition(int level, int index)
{
    if (sym == ODDSYM)
    {
        getSym();
        expression(level, index);
        getTargetCode(LIT, 0, 0);
        getTargetCode(OPR, 0, OP_NEQ);
    }
    else
    {
        expression(level, index);
        int op = sym;
        if (sym != NEQ&&sym != EQ&&sym != LSS&&sym != LE&&sym != GT&&sym != GE)
            printError("error! 条件表达式错误");
        getSym();
        expression(level, index);
        switch (op)
        {
        case NEQ:
            //不相等
            getTargetCode(OPR, 0, OP_NEQ);
            break;
        case EQ:
            //相等
            getTargetCode(OPR, 0, OP_EQ);
            break;
        case LSS:
            //小于
            getTargetCode(OPR, 0, OP_LSS);
            break;
        case LE:
            //小于等于
            getTargetCode(OPR, 0, OP_LE);
            break;
        case GT:
            //大于
            getTargetCode(OPR, 0, OP_GT);
            break;
        case GE:
            //大于等于
            getTargetCode(OPR, 0, OP_GE);
            break;
        }
    }
    return 0;
}


/***
* <表达式> → [+|-]<项>{<加减运算符><项>}
* <项> → <因子>{<乘除运算符><因子>}
* <因子> → <标识符>|<无符号整数>|(<表达式>)
* <加减运符> → +|-
* <乘除运算符> → *|/
* <关系运算符> → =|#|<|<=|>|>=
*/
int expression(int level, int index)//表达式
{
    int op = sym;
    if (sym == PLUS || sym == MINUS)//是否为+/-号，表示符号
    {
        getSym();
    }
    factor(level, index);//调用项方法
/*    if (op == MINUS)
    {
        getTargetCode(LIT, 0, 0);
        getTargetCode(OPR, 0, OP_MINUS);
    }

     //ZYL
    else if(op == PLUS){
        getTargetCode(LIT, 0, 0);
        getTargetCode(OPR, 0, OP_ADD);
    }
  */
    do
    {
        op = sym;
        //<表达式> → [+|-]<项>{<加减运算符><项>}，加减运算符判断
        if (sym == PLUS || sym == MINUS)//加减运算符
        {
            getSym();
            factor(level, index);//调用项
            if (op == PLUS)
                getTargetCode(OPR, 0, OP_ADD);//生成加的目标代码
            else
                getTargetCode(OPR, 0, OP_MINUS);//生成减的目标代码
        }
    }
    while (sym == PLUS || sym == MINUS);
    return 0;
}


/**
*<项> → <因子>{<乘除运算符><因子>}
*/
int factor(int level, int index)
{
    term(level, index);//因子设置
    int op = sym;
    if (op != TIMES&&op != SPLASH)//乘法和除法
        return 0;
    do
    {
        getSym();
        term(level, index);//因子
        if (op == TIMES)
            getTargetCode(OPR, 0, OP_TIMES);//生成乘法目标代码
        else
            getTargetCode(OPR, 0, OP_DIV);//生成除法目标代码
        op = sym;
    }
    while (sym == TIMES || sym == SPLASH);//如果是*或者是·则继续循环
    return 0;
}

/**
*  <因子> → <标识符>|<无符号整数>|(<表达式>)
*/
int term(int level, int index)
{
    if (sym == IDENT)//标识符
    {
        int i = Myfind(0, index, id);
        if (i<0)
        {
            printError("Identifier not found! 未定义变量名");
        }
        if (symTable[i].type == CONST)
            getTargetCode(LIT, 0, symTable[i].value);
        else if (symTable[i].type == VARIABLE)
            getTargetCode(LOD, level - symTable[i].level, numLinkData + symTable[i].address);
        else
        {
            printError("error!");
        }
        getSym();
    }
    else if (sym == NUM)//无符号整数
    {
        getTargetCode(LIT, 0, num);
        getSym();
    }
    else if(sym==LPAREN)//左括号'('
    {
        getSym();
        expression(level, index);//表达式
        if (sym != RPAREN)
            printf(") expected 缺少右括号");
        getSym();
    }
    else
    {
        printError("error! 项定义错误");
    }
    return 0;
}

/**
*从symTable中查找是否变量名已经定义过，如果定义过则返回其在symTable表中的位置
*否则返回-1
*/
int Myfind(int from, int to, const char *name)
{
    for (int i = to - 1; i >= from; i--)
        if (!strcmp(name, symTable[i].name))
            return i;
    return -1;
}

/**
*存放生成的目标代码
*/
void getTargetCode(int op, int l, int a)
{
    PCode pcode(op, l, a);
    cout<<"current PC:"<<pc<<endl;
    code[pc++] = pcode;
    cout<<CodeTable[code[pc-1].op]<<"\t"<<code[pc-1].l<<"\t"<<code[pc-1].a<<endl;
}


/**
*执行生成的目标代码
*/
void interprete()
{
    //返回地址，记录调用该过程时目标程序的断点，即当时的程序地址寄存器P的值。
    //动态链，它是指向调用该过程前正在运行过程的数据段的基地址。
    //静态链，它是指向定义该过程的直接外过程运行时数据段的基地址。
    const static int ret_addr = 0, dynamic_link = 1, static_link = 2;

    //指令寄存器ir
    PCode ir;

    //ip为指令的指针，sp为栈指针，bp为当前procedure基地址
    int ip = 1, sp = 0, bp = 0;

    //定义数据栈
    int stack[1000] = { 0 };

    //基地址栈
    int sp_stack[10];

    //栈顶寄存器，每个过程运行时要为它分配数据区（或称为数据段）
    int sp_top = 0;
    while (ip<pc)
    {
        ir = code[ip++];//指令寄存器存放当前指令
        switch (ir.op)
        {
        case LIT:
        {
            //LIT：将常数放到运栈顶，a域为常数。
            stack[sp++] = ir.a;
            break;
        }
        case LOD:
        {
            //LOD：将变量放到栈顶。a域为变量在所说明层中的相对位置，l为调用层与说明层的层差值。
            if (ir.l == 0)//层差为0
                //基地址bp+偏移地址a中存放栈顶数据
                stack[sp++] = stack[bp + ir.a];
            else
            {
                //直接调用的基地址
                int outer_bp = stack[bp + static_link];

                //一直往外层找，直到层差为0
                while (--ir.l)
                    outer_bp = stack[outer_bp + static_link];
                //将数据取到栈顶
                stack[sp++] = stack[outer_bp + ir.a];
            }
            break;
        }
        case STO:
        {
            //将栈顶的内容送到某变量单元中
            if (ir.l == 0)
                //基地址bp+偏移地址a中存放栈顶数据
                stack[bp + ir.a] = stack[sp - 1];
            else
            {
                //直接调用的基地址
                int outer_bp = stack[bp + static_link];

                //一直往外层找，直到层差为0
                while (--ir.l)
                    outer_bp = stack[outer_bp + static_link];
                //将栈顶的数据存放到指定位置
                stack[outer_bp + ir.a] = stack[sp - 1];
            }
            break;
        }
        case CAL:
        {
            //调用过程的指令。a为被调用过程的目标程序的入中地址，l为层差。

            //将sp+ret_addr存放返回的代码指针，用于返回
            stack[sp + ret_addr] = ip;
            //动态链于静态链存放当前基地址，因为此文法过程是在被调用的时候才声明，因此此文法的动态链与静态链的值是一样的。
            stack[sp + dynamic_link] = bp;
            stack[sp + static_link] = bp;
            ip = ir.a;
            //基地址设置成当前的栈地址
            bp = sp;
            break;
        }
        case INT:
        {
            //为被调用的过程（或主程序）在运行栈中开辟数据区。a域为开辟的个数。

            //将新分配的程序的基地址存放到sp_stack中，并为为其在stack中开辟a个位置。
            sp_stack[sp_top++] = sp;
            sp += ir.a;
            break;
        }
        case JMP:
        {
            //无条件转移指令，a为转向地址。
            ip = ir.a;
            break;
        }
        case JPC:
        {
            //条件转移指令，当栈顶的布尔值为非真时，转向a域的地址，否则顺序执行。
            if (stack[sp - 1] == 0)
                ip = ir.a;
            break;
        }
        case OPR:
        {
            //关系和算术运算。具体操作由a域给出。运算对象为栈顶和次顶的内容进行运算，结果存放在次顶。a域为0时是退出数据区。
            switch (ir.a)
            //对具体的操作进行判断，
            {
            case OP_RET:
            {
                //当一个procedure调用完成之后清除为其分配的内存

                //ip指向调用其的指令
                ip = stack[bp + ret_addr];
                //数据段基地址变为调用过程的直接过程的基地址
                bp = stack[bp + dynamic_link];
                //栈地址退回到调用的位置。即下面的调用会对先前已经结束的数据进行覆盖
                sp = sp_stack[--sp_top];
                //如果sp_top<=0说明对所有过程分配的存储空间均回收，程序运行结束。
                if (sp_top <= 0)
                {
                    cout<<"program exited normally!\n"<<endl;;
                    return;
                }
                break;
            }
            case OP_ADD:
            {
                //栈顶的值加上去次顶的值并将结果存放到次顶
                stack[sp - 2] = stack[sp - 1] + stack[sp - 2];
                sp--;
                break;
            }
            case OP_MINUS:
            {
                //栈顶元素减去次顶元素并将结果存放到次顶
                stack[sp - 2] = stack[sp - 1] - stack[sp - 2];
                sp--;
                break;
            }
            case OP_TIMES:
            {
                //栈顶元素乘以次顶元素并将结果存放到次顶
                stack[sp - 2] = stack[sp - 1] * stack[sp - 2];
                sp--;
                break;
            }
            case OP_DIV:
            {
                //栈顶元素除以次顶元素并将结果存放到次顶中
                stack[sp - 2] = stack[sp - 2] / stack[sp - 1];
                sp--;
                break;
            }
            case OP_NEQ:
            {
                //比较栈顶元素与次顶元素是否不相等，不相等则返回1
                stack[sp - 2] = (stack[sp - 2] != stack[sp - 1]) ? 1 : 0;
                sp--;
                break;
            }
            case OP_EQ:
            {
                //比较栈顶元素与次顶元素是否相等，相等则返回1
                stack[sp - 2] = (stack[sp - 2] == stack[sp - 1]) ? 1 : 0;
                sp--;
                break;
            }
            case OP_LSS:
            {
                //比较栈顶元素是否小于次顶元素，小于则返回1
                stack[sp - 2] = (stack[sp - 2]<stack[sp - 1]) ? 1 : 0;
                sp--;
                break;
            }
            case OP_LE:
            {
                //比较栈顶元素是否小于等于次顶元素，小于等于则返回1
                stack[sp - 2] = (stack[sp - 2] <= stack[sp - 1]) ? 1 : 0;
                sp--;
                break;
            }
            case OP_GT:
            {
                //比较栈顶元素是否大于次顶元素，大于则返回1
                stack[sp - 2] = (stack[sp - 2]>stack[sp - 1]) ? 1 : 0;
                sp--;
                break;
            }
            case OP_GE:
            {
                //比较栈顶元素是否大于等于次顶元素，大于等于则返回1
                stack[sp - 2] = (stack[sp - 2] >= stack[sp - 1]) ? 1 : 0;
                sp--;
                break;
            }
            case OP_READ:
            {
                //读入用户输入的数据
                cout << "Please input a number:" << endl;
                cin >> stack[sp++];
                break;
            }
            case OP_WRITE:
            {
                //将栈顶的结果输出
                cout << "program display:"<<stack[sp - 1] << endl;
                break;
            }
            default:
            {
                printf("Unexpected operation!\n");
                return;
            }
            }
            break;
        }
        default:
            printf("Unexpected instruction!\n");
            return;
        }
        cout<<"栈：";
        for(int i=0;i<sp;i++){
        cout<<stack[i]<<"";

        }
        cout<<endl;
    }

}

/**
*打印表格
*/
void printSymTable(){
    cout<<"-----------------------------table----------------------------"<<endl;
    for(int i=0;i<count_sym;i++){
        if(symTable[i].type==0){
            cout<<"NAME:"<<symTable[i].name<<"\t\t";
            cout<<"TYPE:CONST\t\t";
            cout<<"VAL:"<<symTable[i].value<<endl;
        }
        else if(symTable[i].type==1){
            cout<<"NAME:"<<symTable[i].name<<"\t\t";
            cout<<"TYPE:VAR\t\t";
            cout<<"LEVEL:"<<symTable[i].level<<"\t\t";
            cout<<"ADDR:"<<symTable[i].address<<endl;
        }
        else if(symTable[i].type==2){
            cout<<"NAME:"<<symTable[i].name<<"\t\t";
            cout<<"TYPE:PROCEDURE\t\t";
            cout<<"LEVEL:"<<symTable[i].level<<"\t\t";
            cout<<"ADDR:"<<symTable[i].address<<endl;
        }
    }

}


/**
*打印ID，SYM和NUM
*/
void printArray(){
    cout<<"-----------------------------Array----------------------------"<<endl;
    cout<<"ID:"<<endl;
    for(int i=0;i<idArrayCount;i++){
        cout<<IDArray[i]<<"  ";
    }
    cout<<endl<<"NUM:"<<endl;
    for(int i=0;i<numArrayCount;i++){
        cout<<NUMArray[i]<<" ";
    }
        cout<<endl<<"SYM:"<<endl;
    for(int i=0;i<symArrayCount;i++){
        cout<<SYMArray[i]<<" ";
    }
    cout<<endl;
}

int main()
{
    f = fopen("test.txt","r");
    getSym();//判断txt的文件是否存在内容；
    block(0, 0);//表示第0层，index为0
    printArray();
    printSymTable();
    cout<<"---------------------------TargetCode--------------------------"<<endl;
    for (int i = 1; i<pc; i++)
    {
        cout << i << ":\t" << CodeTable[code[i].op] << "\t" << code[i].l << "\t" << code[i].a << endl;
    }
    cout<<"程序运行结果："<<endl;
    interprete();
    return 0;
}
