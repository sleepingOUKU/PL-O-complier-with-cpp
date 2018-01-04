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

//�жϹؼ�������
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
    while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')//���ڳ�ȥ����Ҫ�Ŀո�ͻ��е�
    {
        if ((ch = fgetc(in)) == EOF) //���Դ���ȫ������
        {
            cout<<"\n::read finish::"<<endl;
            return -1;
        }
        if (ch == '\n')//����������һ
            line++;
    }
    if (isalpha(ch))//�ж��Ƿ�Ϊ��ĸ
    {
        while (isalpha(ch) || isdigit(ch))//��ȡһ�������Ĵʻ㣬��abc�����ǹؼ���
        {
            if (pos >= maxIdLength)
                return -1;
            buf[pos++] = ch;
            ch = fgetc(in);
        }
        ungetc(ch, in);//���˵����뻺����
        buf[pos++] = '\0';
        sym = findKeyword(buf);

        if (sym<0)
        {
            sym = IDENT;//�û������ʶ��
            //cout<<"IDENT:"<<buf<<endl;
            strcpy(id, buf);

            //�ж��Ƿ��Ѿ�����ID����
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
    else if (isdigit(ch))//�û���ֵ����
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
*���ֳ��򡵡� [<����˵������>][<����˵������>][<����˵������>]����䡵
*/
int block(int level, int index)
{
    int count = 0, dx = 0;
    int tpc = pc;
    getTargetCode(JMP, 0, 0);//��һ��Ϊ��תָ���ת��ַδ֪
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
        flag = true;//����˵������level+1
        getSym();
        int px = count + index;
        count += procDeclaration(level + 1, px);
    }
    if (!flag)
        pc--;//û�����д���
    else
    {
        code[tpc].a = pc;//�������������ת��ָ��ĵ�ַ��
//       cout<<CodeTable[code[tpc].op]<<"\t"<<code[tpc].l<<"\t"<<code[tpc].a<<endl;
    }
    getTargetCode(INT, 0, numLinkData + dx);
    statement(level, index + count);//������䲿��
    getTargetCode(OPR, 0, OP_RET);
    return count;
}

/**
*<����˵������> �� CONST<��������>{ ,<��������>}
*<��������> �� <��ʶ��>=<�޷�������>
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
            printError("identifier expected! δ�����û���");
        if (Myfind(0, index + count, id) >= 0)
            printError("duplicated identifier! �ظ�����");
        strcpy(sb.name, id);
        getSym();
        if (sym != EQ)
            printError("a '=' expected! ȱ��'='");
        getSym();
        if (sym != NUM)
            printError("number expected! ȱ�ٳ�ʼ��");
        sb.value = num;

        symTable[index + count++] = sb;//��ӵ�symTable�С�
        count_sym++;
        getSym();
        if (!(sym == COMMA || sym == SEMICOLON))
            printError("comma or semicolon expected! ȱ�ٽ�������");
    }
    while (sym != SEMICOLON);  //����Ƕ������ʾΪ������塣
    getSym();
    return count;
}

/**
* <����˵������> �� VAR<��ʶ��>{ ,<��ʶ��>}��
* <��ʶ��> �� <��ĸ>{<��ĸ>|<����>}
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
            printError("identifier expected! δ�����û���");
        if (Myfind(0, index + count, id) >= 0)
            printError("duplicated expected! �ظ�����");
        strcpy(sb.name, id);

        symTable[index + count++] = sb;
        count_sym++;
        sb.address++;
        getSym();
        if (!(sym == COMMA || sym == SEMICOLON))
            printError("comma or semicolon expected! ȱ�ٽ�������");
        tsym = sym;
        getSym();
    }
    while (tsym != SEMICOLON);
    return count;
}

/**
*<����˵������> �� <�����ײ�><�̶ֳ�>��{<����˵������>}
*/
int procDeclaration(int level, int index)
{
    int count = 0;//��¼level�ж���ı�����
    if (sym != IDENT)
        printError("identifier expected! δ���������");
    Symbol sb;
    strcpy(sb.name, id);
    sb.type = PROCEDURE;
    sb.level = level - 1;//?
    sb.address = pc;

    symTable[index + count++] = sb;
    count_sym++;
    getSym();
    if (sym != SEMICOLON)
        printError("semicolon expected! ȱ�ٷֺ�");
    getSym();
    block(level, index + count);
    if (sym != SEMICOLON)
        printError("semicolon expected! ȱ�ٷֺ�");
    getSym();
    if (sym == PROCSYM)//�����procedure���ٴε���procedureDeclaration
    {
        getSym();
        count += procDeclaration(level, index + count);
    }
    return count + 1;
}

/**
*<���> �� <��ֵ���>|<�������>|<����ѭ�����>|<���̵������>|<�����>|<д���>|<�������>|<��>
*/
int statement(int level, int index)
{
    if (sym == IFSYM)//if����
    {
        getSym();
        ifStatement(level, index);
    }
    else if (sym == WHILESYM)//while����
    {
        getSym();
        whileStatement(level, index);
    }
    else if (sym == CALLSYM)//call����
    {
        getSym();
        callStatement(level, index);
    }
    else if (sym == WRITESYM)//д������������Ļ���
    {
        getSym();
        writeStatement(level, index);
    }
    else if (sym == READSYM)//���û������ȡ����
    {
        getSym();
        readStatement(level, index);
    }
    else if (sym == BEGINSYM)//������䣬begin����
    {
        getSym();
        compositeStatement(level, index);
    }
    else if (sym == IDENT)//<��ֵ���>
    {
        assignStatement(level, index);
    }
    else
        return 0;
    return 0;
}

/**
*if�����ж�<�������> �� if<����>then<���>
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
*whileѭ��<����ѭ�����> �� while<����>do<���>
*/
int whileStatement(int level, int index)
{
    int cpc = pc;//��¼��ǰwhile�ĳ�ʼλ��
    condition(level, index);//������������
    int jpc = pc;
    getTargetCode(JPC, 0, 0);
    if (sym != DOSYM)
    {
        printError("do expected! ȱ�ٱ�ʶ��'do'");
    }
    getSym();
    statement(level, index);//��������statement <���>
    getTargetCode(JMP, 0, cpc);//���JMPָ�JMP��ַΪwhile�ĳ�ʼ��ַ��
    code[jpc].a = pc;
    return 0;
}

/**
*<���̵������> �� call<��ʶ��>
*/
int callStatement(int level, int index)
{
    if (sym != IDENT)//�ж��Ƿ�Ϊ��ʶ��
        printError("syntax error! ��ʼ������");
    int i = Myfind(0, index, id);//�ҵ���ͬ���Ƶı�����symTable�е�λ��
    if (i<0)
        printError("identifier not found! δ�����û���");
    if (symTable[i].type != PROCEDURE)
        printError("syntax error! ���ô���");


    //call��������genCode
    getTargetCode(CAL, level - symTable[i].level, symTable[i].address);//��symTable�еĵ�ַȡ����
    getSym();
    return 0;
}

/**
*read�������� <�����> �� read(<��ʶ��>{ ��<��ʶ��>})
*/
int readStatement(int level, int index)
{
    if (sym != LPAREN)
        printError(" ( expected ȱ��'('");
    getSym();
    while (sym != RPAREN)//����Ϊ�����ŵ�ʱ��˵��readδ��
    {
        if (sym != IDENT)//�����û��Զ�����������
            printError("variable expected! ȱ���Զ������");
        int i = Myfind(0, index, id);//Ѱ�ұ����Ƿ��Ѿ������
        if (i<0)
            printError("identifier not found! �Ҳ�������");
        if (symTable[i].type != VARIABLE)//�ж��Ƿ�Ϊ����
            printError("variable expected! ���Ǳ���");
        getTargetCode(OPR, 0, OP_READ);//����������ݴ���ջ��
        getTargetCode(STO, level - symTable[i].level, symTable[i].address + numLinkData);//��ջ���������͵�ĳ������Ԫ��
        getSym();
        if (sym != COMMA&&sym != RPAREN)//��������sym��Ϊ���Ż����������ŵ�ʱ�򣬳��ִ���
            printError("syntax error! Readȱʧ�����Ż�ֺ�");
    }
    getSym();
    return 0;
}

/**
*<д���> �� write(<��ʶ��>{��<��ʶ��>})
*/
int writeStatement(int level, int index)
{
    if (sym != LPAREN)
        printError(" ( expected ȱ�������š�");
    getSym();
    while (sym != RPAREN)
    {
        expression(level, index);
        getTargetCode(OPR, 0, OP_WRITE);
        if (sym != COMMA&&sym != RPAREN)//��������sym��Ϊ���Ż����������ŵ�ʱ�򣬳��ִ���
            printError("syntax error! Readȱʧ�����Ż�ֺ�");
    }
    getSym();
    return 0;
}

/**
*<�������> �� begin<���>{ ��<���>}<end>
*/
int compositeStatement(int level, int index)
{
    statement(level, index);
    while (sym == SEMICOLON)
    {
        getSym();
        statement(level, index);//�������
    }
    if (sym != ENDSYM)
        printError("end expected!");
    getSym();
    return 0;
}

/**
*������ֵ���<��ֵ���> �� <��ʶ��>:=<���ʽ>
*/
int assignStatement(int level, int index)
{
    int i = Myfind(0, index, id);
    if (i<0)
    {
        printError("Variable not found! δ�������");
    }
    if (symTable[i].type == CONST)
        printError("constant can't be a r-value! �����޷���ֵ");
    getSym();
    if (sym != ASSIGN)
    {
        printError(":= expected!");
    }
    getSym();
    expression(level, index);
    //������Ľ����ŵ�SymTable������λ�õĲ��Ϊlevel - symTable[i].level�������ڲ��ַΪnumLinkData + symTable[i].address
    getTargetCode(STO, level - symTable[i].level, numLinkData + symTable[i].address);
    return 0;
}

/**
*<����> �� <���ʽ><��ϵ�����><���ʽ>|odd<���ʽ>
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
            printError("error! �������ʽ����");
        getSym();
        expression(level, index);
        switch (op)
        {
        case NEQ:
            //�����
            getTargetCode(OPR, 0, OP_NEQ);
            break;
        case EQ:
            //���
            getTargetCode(OPR, 0, OP_EQ);
            break;
        case LSS:
            //С��
            getTargetCode(OPR, 0, OP_LSS);
            break;
        case LE:
            //С�ڵ���
            getTargetCode(OPR, 0, OP_LE);
            break;
        case GT:
            //����
            getTargetCode(OPR, 0, OP_GT);
            break;
        case GE:
            //���ڵ���
            getTargetCode(OPR, 0, OP_GE);
            break;
        }
    }
    return 0;
}


/***
* <���ʽ> �� [+|-]<��>{<�Ӽ������><��>}
* <��> �� <����>{<�˳������><����>}
* <����> �� <��ʶ��>|<�޷�������>|(<���ʽ>)
* <�Ӽ��˷�> �� +|-
* <�˳������> �� *|/
* <��ϵ�����> �� =|#|<|<=|>|>=
*/
int expression(int level, int index)//���ʽ
{
    int op = sym;
    if (sym == PLUS || sym == MINUS)//�Ƿ�Ϊ+/-�ţ���ʾ����
    {
        getSym();
    }
    factor(level, index);//�������
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
        //<���ʽ> �� [+|-]<��>{<�Ӽ������><��>}���Ӽ�������ж�
        if (sym == PLUS || sym == MINUS)//�Ӽ������
        {
            getSym();
            factor(level, index);//������
            if (op == PLUS)
                getTargetCode(OPR, 0, OP_ADD);//���ɼӵ�Ŀ�����
            else
                getTargetCode(OPR, 0, OP_MINUS);//���ɼ���Ŀ�����
        }
    }
    while (sym == PLUS || sym == MINUS);
    return 0;
}


/**
*<��> �� <����>{<�˳������><����>}
*/
int factor(int level, int index)
{
    term(level, index);//��������
    int op = sym;
    if (op != TIMES&&op != SPLASH)//�˷��ͳ���
        return 0;
    do
    {
        getSym();
        term(level, index);//����
        if (op == TIMES)
            getTargetCode(OPR, 0, OP_TIMES);//���ɳ˷�Ŀ�����
        else
            getTargetCode(OPR, 0, OP_DIV);//���ɳ���Ŀ�����
        op = sym;
    }
    while (sym == TIMES || sym == SPLASH);//�����*�����ǡ������ѭ��
    return 0;
}

/**
*  <����> �� <��ʶ��>|<�޷�������>|(<���ʽ>)
*/
int term(int level, int index)
{
    if (sym == IDENT)//��ʶ��
    {
        int i = Myfind(0, index, id);
        if (i<0)
        {
            printError("Identifier not found! δ���������");
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
    else if (sym == NUM)//�޷�������
    {
        getTargetCode(LIT, 0, num);
        getSym();
    }
    else if(sym==LPAREN)//������'('
    {
        getSym();
        expression(level, index);//���ʽ
        if (sym != RPAREN)
            printf(") expected ȱ��������");
        getSym();
    }
    else
    {
        printError("error! ������");
    }
    return 0;
}

/**
*��symTable�в����Ƿ�������Ѿ�����������������򷵻�����symTable���е�λ��
*���򷵻�-1
*/
int Myfind(int from, int to, const char *name)
{
    for (int i = to - 1; i >= from; i--)
        if (!strcmp(name, symTable[i].name))
            return i;
    return -1;
}

/**
*������ɵ�Ŀ�����
*/
void getTargetCode(int op, int l, int a)
{
    PCode pcode(op, l, a);
    cout<<"current PC:"<<pc<<endl;
    code[pc++] = pcode;
    cout<<CodeTable[code[pc-1].op]<<"\t"<<code[pc-1].l<<"\t"<<code[pc-1].a<<endl;
}


/**
*ִ�����ɵ�Ŀ�����
*/
void interprete()
{
    //���ص�ַ����¼���øù���ʱĿ�����Ķϵ㣬����ʱ�ĳ����ַ�Ĵ���P��ֵ��
    //��̬��������ָ����øù���ǰ�������й��̵����ݶεĻ���ַ��
    //��̬��������ָ����ù��̵�ֱ�����������ʱ���ݶεĻ���ַ��
    const static int ret_addr = 0, dynamic_link = 1, static_link = 2;

    //ָ��Ĵ���ir
    PCode ir;

    //ipΪָ���ָ�룬spΪջָ�룬bpΪ��ǰprocedure����ַ
    int ip = 1, sp = 0, bp = 0;

    //��������ջ
    int stack[1000] = { 0 };

    //����ַջ
    int sp_stack[10];

    //ջ���Ĵ�����ÿ����������ʱҪΪ�����������������Ϊ���ݶΣ�
    int sp_top = 0;
    while (ip<pc)
    {
        ir = code[ip++];//ָ��Ĵ�����ŵ�ǰָ��
        switch (ir.op)
        {
        case LIT:
        {
            //LIT���������ŵ���ջ����a��Ϊ������
            stack[sp++] = ir.a;
            break;
        }
        case LOD:
        {
            //LOD���������ŵ�ջ����a��Ϊ��������˵�����е����λ�ã�lΪ���ò���˵����Ĳ��ֵ��
            if (ir.l == 0)//���Ϊ0
                //����ַbp+ƫ�Ƶ�ַa�д��ջ������
                stack[sp++] = stack[bp + ir.a];
            else
            {
                //ֱ�ӵ��õĻ���ַ
                int outer_bp = stack[bp + static_link];

                //һֱ������ң�ֱ�����Ϊ0
                while (--ir.l)
                    outer_bp = stack[outer_bp + static_link];
                //������ȡ��ջ��
                stack[sp++] = stack[outer_bp + ir.a];
            }
            break;
        }
        case STO:
        {
            //��ջ���������͵�ĳ������Ԫ��
            if (ir.l == 0)
                //����ַbp+ƫ�Ƶ�ַa�д��ջ������
                stack[bp + ir.a] = stack[sp - 1];
            else
            {
                //ֱ�ӵ��õĻ���ַ
                int outer_bp = stack[bp + static_link];

                //һֱ������ң�ֱ�����Ϊ0
                while (--ir.l)
                    outer_bp = stack[outer_bp + static_link];
                //��ջ�������ݴ�ŵ�ָ��λ��
                stack[outer_bp + ir.a] = stack[sp - 1];
            }
            break;
        }
        case CAL:
        {
            //���ù��̵�ָ�aΪ�����ù��̵�Ŀ���������е�ַ��lΪ��

            //��sp+ret_addr��ŷ��صĴ���ָ�룬���ڷ���
            stack[sp + ret_addr] = ip;
            //��̬���ھ�̬����ŵ�ǰ����ַ����Ϊ���ķ��������ڱ����õ�ʱ�����������˴��ķ��Ķ�̬���뾲̬����ֵ��һ���ġ�
            stack[sp + dynamic_link] = bp;
            stack[sp + static_link] = bp;
            ip = ir.a;
            //����ַ���óɵ�ǰ��ջ��ַ
            bp = sp;
            break;
        }
        case INT:
        {
            //Ϊ�����õĹ��̣���������������ջ�п�����������a��Ϊ���ٵĸ�����

            //���·���ĳ���Ļ���ַ��ŵ�sp_stack�У���ΪΪ����stack�п���a��λ�á�
            sp_stack[sp_top++] = sp;
            sp += ir.a;
            break;
        }
        case JMP:
        {
            //������ת��ָ�aΪת���ַ��
            ip = ir.a;
            break;
        }
        case JPC:
        {
            //����ת��ָ���ջ���Ĳ���ֵΪ����ʱ��ת��a��ĵ�ַ������˳��ִ�С�
            if (stack[sp - 1] == 0)
                ip = ir.a;
            break;
        }
        case OPR:
        {
            //��ϵ���������㡣���������a��������������Ϊջ���ʹζ������ݽ������㣬�������ڴζ���a��Ϊ0ʱ���˳���������
            switch (ir.a)
            //�Ծ���Ĳ��������жϣ�
            {
            case OP_RET:
            {
                //��һ��procedure�������֮�����Ϊ�������ڴ�

                //ipָ��������ָ��
                ip = stack[bp + ret_addr];
                //���ݶλ���ַ��Ϊ���ù��̵�ֱ�ӹ��̵Ļ���ַ
                bp = stack[bp + dynamic_link];
                //ջ��ַ�˻ص����õ�λ�á�������ĵ��û����ǰ�Ѿ����������ݽ��и���
                sp = sp_stack[--sp_top];
                //���sp_top<=0˵�������й��̷���Ĵ洢�ռ�����գ��������н�����
                if (sp_top <= 0)
                {
                    cout<<"program exited normally!\n"<<endl;;
                    return;
                }
                break;
            }
            case OP_ADD:
            {
                //ջ����ֵ����ȥ�ζ���ֵ���������ŵ��ζ�
                stack[sp - 2] = stack[sp - 1] + stack[sp - 2];
                sp--;
                break;
            }
            case OP_MINUS:
            {
                //ջ��Ԫ�ؼ�ȥ�ζ�Ԫ�ز��������ŵ��ζ�
                stack[sp - 2] = stack[sp - 1] - stack[sp - 2];
                sp--;
                break;
            }
            case OP_TIMES:
            {
                //ջ��Ԫ�س��Դζ�Ԫ�ز��������ŵ��ζ�
                stack[sp - 2] = stack[sp - 1] * stack[sp - 2];
                sp--;
                break;
            }
            case OP_DIV:
            {
                //ջ��Ԫ�س��Դζ�Ԫ�ز��������ŵ��ζ���
                stack[sp - 2] = stack[sp - 2] / stack[sp - 1];
                sp--;
                break;
            }
            case OP_NEQ:
            {
                //�Ƚ�ջ��Ԫ����ζ�Ԫ���Ƿ���ȣ�������򷵻�1
                stack[sp - 2] = (stack[sp - 2] != stack[sp - 1]) ? 1 : 0;
                sp--;
                break;
            }
            case OP_EQ:
            {
                //�Ƚ�ջ��Ԫ����ζ�Ԫ���Ƿ���ȣ�����򷵻�1
                stack[sp - 2] = (stack[sp - 2] == stack[sp - 1]) ? 1 : 0;
                sp--;
                break;
            }
            case OP_LSS:
            {
                //�Ƚ�ջ��Ԫ���Ƿ�С�ڴζ�Ԫ�أ�С���򷵻�1
                stack[sp - 2] = (stack[sp - 2]<stack[sp - 1]) ? 1 : 0;
                sp--;
                break;
            }
            case OP_LE:
            {
                //�Ƚ�ջ��Ԫ���Ƿ�С�ڵ��ڴζ�Ԫ�أ�С�ڵ����򷵻�1
                stack[sp - 2] = (stack[sp - 2] <= stack[sp - 1]) ? 1 : 0;
                sp--;
                break;
            }
            case OP_GT:
            {
                //�Ƚ�ջ��Ԫ���Ƿ���ڴζ�Ԫ�أ������򷵻�1
                stack[sp - 2] = (stack[sp - 2]>stack[sp - 1]) ? 1 : 0;
                sp--;
                break;
            }
            case OP_GE:
            {
                //�Ƚ�ջ��Ԫ���Ƿ���ڵ��ڴζ�Ԫ�أ����ڵ����򷵻�1
                stack[sp - 2] = (stack[sp - 2] >= stack[sp - 1]) ? 1 : 0;
                sp--;
                break;
            }
            case OP_READ:
            {
                //�����û����������
                cout << "Please input a number:" << endl;
                cin >> stack[sp++];
                break;
            }
            case OP_WRITE:
            {
                //��ջ���Ľ�����
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
        cout<<"ջ��";
        for(int i=0;i<sp;i++){
        cout<<stack[i]<<"";

        }
        cout<<endl;
    }

}

/**
*��ӡ���
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
*��ӡID��SYM��NUM
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
    getSym();//�ж�txt���ļ��Ƿ�������ݣ�
    block(0, 0);//��ʾ��0�㣬indexΪ0
    printArray();
    printSymTable();
    cout<<"---------------------------TargetCode--------------------------"<<endl;
    for (int i = 1; i<pc; i++)
    {
        cout << i << ":\t" << CodeTable[code[i].op] << "\t" << code[i].l << "\t" << code[i].a << endl;
    }
    cout<<"�������н����"<<endl;
    interprete();
    return 0;
}
