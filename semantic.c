/*----------------------------------------------------------------------------
 Note: the code in this file is not to be shared with anyone or posted online.
 (c) Rida Bazzi, 2015, Adam Doupe, 2015
 ----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "syntax.h"

/* ------------------------------------------------------- */
/* -------------------- LEXER SECTION -------------------- */
/* ------------------------------------------------------- */

#define KEYWORDS  11

typedef enum
{
    END_OF_FILE = -1, VAR = 1, WHILE, INT, REAL, STRING, BOOLEAN,
    TYPE, LONG, DO, CASE, SWITCH,
    PLUS, MINUS, DIV, MULT, EQUAL,
    COLON, COMMA, SEMICOLON,
    LBRAC, RBRAC, LPAREN, RPAREN, LBRACE, RBRACE,
    NOTEQUAL, GREATER, LESS, LTEQ, GTEQ, DOT,
    ID, NUM, REALNUM,
    ERROR
} token_type;

const char *reserved[] = {"",
    "VAR", "WHILE", "INT", "REAL", "STRING", "BOOLEAN",
    "TYPE", "LONG", "DO", "CASE", "SWITCH",
    "+", "-", "/", "*", "=",
    ":", ",", ";",
    "[", "]", "(", ")", "{", "}",
    "<>", ">", "<", "<=", ">=", ".",
    "ID", "NUM", "REALNUM",
    "ERROR"
};

// Global Variables associated with the next input token
#define MAX_TOKEN_LENGTH 100
char token[MAX_TOKEN_LENGTH]; // token string
token_type t_type; // token type
bool activeToken = false;
int tokenLength;
int line_no = 1;

void skipSpace()
{
    char c;
    
    c = getchar();
    line_no += (c == '\n');
    while (!feof(stdin) && isspace(c))
    {
        c = getchar();
        line_no += (c == '\n');
    }
    ungetc(c, stdin);
}

int isKeyword(char *s)
{
    int i;
    
    for (i = 1; i <= KEYWORDS; i++)
    {
        if (strcmp(reserved[i], s) == 0)
        {
            return i;
        }
    }
    return false;
}

/*
 * ungetToken() simply sets a flag so that when getToken() is called
 * the old t_type is returned and the old token is not overwritten.
 * NOTE: BETWEEN ANY TWO SEPARATE CALLS TO ungetToken() THERE MUST BE
 * AT LEAST ONE CALL TO getToken(). CALLING TWO ungetToken() WILL NOT
 * UNGET TWO TOKENS
 */
void ungetToken()
{
    activeToken = true;
}

token_type scan_number()
{
    char c;
    
    c = getchar();
    if (isdigit(c))
    {
        // First collect leading digits before dot
        // 0 is a NUM by itself
        if (c == '0')
        {
            token[tokenLength] = c;
            tokenLength++;
            token[tokenLength] = '\0';
        }
        else
        {
            while (isdigit(c))
            {
                token[tokenLength] = c;
                tokenLength++;
                c = getchar();
            }
            ungetc(c, stdin);
            token[tokenLength] = '\0';
        }
        // Check if leading digits are integer part of a REALNUM
        c = getchar();
        if (c == '.')
        {
            c = getchar();
            if (isdigit(c))
            {
                token[tokenLength] = '.';
                tokenLength++;
                while (isdigit(c))
                {
                    token[tokenLength] = c;
                    tokenLength++;
                    c = getchar();
                }
                token[tokenLength] = '\0';
                if (!feof(stdin))
                {
                    ungetc(c, stdin);
                }
                return REALNUM;
            }
            else
            {
                ungetc(c, stdin);
                c = '.';
                ungetc(c, stdin);
                return NUM;
            }
        }
        else
        {
            ungetc(c, stdin);
            return NUM;
        }
    }
    else
    {
        return ERROR;
    }
}

token_type scan_id_or_keyword()
{
    token_type the_type;
    int k;
    char c;
    
    c = getchar();
    if (isalpha(c))
    {
        while (isalnum(c))
        {
            token[tokenLength] = c;
            tokenLength++;
            c = getchar();
        }
        if (!feof(stdin))
        {
            ungetc(c, stdin);
        }
        token[tokenLength] = '\0';
        k = isKeyword(token);
        if (k == 0)
        {
            the_type = ID;
        }
        else
        {
            the_type = (token_type) k;
        }
        return the_type;
    }
    else
    {
        return ERROR;
    }
}

token_type getToken()
{
    char c;
    
    if (activeToken)
    {
        activeToken = false;
        return t_type;
    }
    skipSpace();
    tokenLength = 0;
    c = getchar();
    switch (c)
    {
        case '.': return DOT;
        case '+': return PLUS;
        case '-': return MINUS;
        case '/': return DIV;
        case '*': return MULT;
        case '=': return EQUAL;
        case ':': return COLON;
        case ',': return COMMA;
        case ';': return SEMICOLON;
        case '[': return LBRAC;
        case ']': return RBRAC;
        case '(': return LPAREN;
        case ')': return RPAREN;
        case '{': return LBRACE;
        case '}': return RBRACE;
        case '<':
            c = getchar();
            if (c == '=')
            {
                return LTEQ;
            }
            else if (c == '>')
            {
                return NOTEQUAL;
            }
            else
            {
                ungetc(c, stdin);
                return LESS;
            }
        case '>':
            c = getchar();
            if (c == '=')
            {
                return GTEQ;
            }
            else
            {
                ungetc(c, stdin);
                return GREATER;
            }
        default:
            if (isdigit(c))
            {
                ungetc(c, stdin);
                return scan_number();
            }
            else if (isalpha(c))
            {
                ungetc(c, stdin);
                return scan_id_or_keyword();
            }
            else if (c == EOF)
            {
                return END_OF_FILE;
            }
            else
            {
                return ERROR;
            }
    }
}

/* ----------------------------------------------------------------- */
/* -------------------- SYNTAX ANALYSIS SECTION -------------------- */
/* ----------------------------------------------------------------- */

void syntax_error(const char* msg)
{
    printf("Syntax error while parsing %s line %d\n", msg, line_no);
    exit(1);
}

/* -------------------- PRINTING PARSE TREE -------------------- */
void print_parse_tree(struct programNode* program)
{
    print_decl(program->decl);
    print_body(program->body);
}

void print_decl(struct declNode* dec)
{
    if (dec->type_decl_section != NULL)
    {
        print_type_decl_section(dec->type_decl_section);
    }
    if (dec->var_decl_section != NULL)
    {
        print_var_decl_section(dec->var_decl_section);
    }
}

void print_body(struct bodyNode* body)
{
    //printf("{\n");
    print_stmt_list(body->stmt_list);
    //printf("}\n");
}

void print_var_decl_section(struct var_decl_sectionNode* varDeclSection)
{
    var_section_accessed = 1;
    //printf("VAR\n");
    if (varDeclSection->var_decl_list != NULL)
    {
        print_var_decl_list(varDeclSection->var_decl_list);
    }
}

void print_var_decl_list(struct var_decl_listNode* varDeclList)
{
    print_var_decl(varDeclList->var_decl);
    if (varDeclList->var_decl_list != NULL)
    {
        print_var_decl_list(varDeclList->var_decl_list);
    }
}

void print_var_decl(struct var_declNode* varDecl)
{
    print_id_list(varDecl->id_list);
    //printf(": ");
    print_type_name(varDecl->type_name);
    //printf(";\n");
}

void print_type_decl_section(struct type_decl_sectionNode* typeDeclSection)
{
    //printf("TYPE\n");
    if (typeDeclSection->type_decl_list != NULL)
    {
        print_type_decl_list(typeDeclSection->type_decl_list);
    }
}

void print_type_decl_list(struct type_decl_listNode* typeDeclList)
{
    print_type_decl(typeDeclList->type_decl);
    if (typeDeclList->type_decl_list != NULL)
    {
        print_type_decl_list(typeDeclList->type_decl_list);
    }
}

void print_type_decl(struct type_declNode* typeDecl)
{
    print_id_list(typeDecl->id_list);
    //printf(": ");
    print_type_name(typeDecl->type_name);
    //printf(";\n");
}

void print_type_name(struct type_nameNode* typeName)
{
    
    if (var_section_accessed == 1) {

        if (typeName->type != ID)
        {
            // printf("%s ", reserved[typeName->type]);
            var_assigns[var_assign_count] = reserved[typeName->type];
            var_assign_count+=1;
        }
        else
        {
            // printf("%s ", typeName->id);
            var_assigns[var_assign_count] = typeName->id;
            var_assign_count+=1;
        }
    }
    
}

void print_id_list(struct id_listNode* idList)
{
    //printf("%s ", idList->id);
    // printf("CURRENT ID IS: %s\n", idList->id);
    if (var_section_accessed == 0) {
        type_ids[type_id_count] = idList->id;
        type_id_count+=1;
    }
    else if (var_section_accessed == 1) {
        // printf("CURRENT ID IS: %s\n", idList->id);
        var_ids[var_id_count] = idList->id;
        var_id_count+=1;
    }

    if (idList->id_list != NULL)
    {
        //printf(", ");
        print_id_list(idList->id_list);
    }
}

void print_stmt_list(struct stmt_listNode* stmt_list)
{
    print_stmt(stmt_list->stmt);
    if (stmt_list->stmt_list != NULL)
    {
        print_stmt_list(stmt_list->stmt_list);
    }
    
}

void print_assign_stmt(struct assign_stmtNode* assign_stmt)
{
    // printf("%s ", assign_stmt->id);
    //printf("= ");
    print_expression_prefix(assign_stmt->expr);
    //printf("; \n");
}

void print_stmt(struct stmtNode* stmt)
{
    switch (stmt->stmtType)
    {
        case ASSIGN:
            print_assign_stmt(stmt->assign_stmt);
            break;
        case WHILE:
            print_while_stmt(stmt->while_stmt);
            break;
        case DO:
            print_do_stmt(stmt->while_stmt);
            break;
        case SWITCH:
            print_switch_stmt(stmt->switch_stmt);
            break;
    }
}

void print_expression_prefix(struct exprNode* expr)
{
    if (expr->tag == EXPR)
    {
        //printf("%s ", reserved[expr->op]);
        print_expression_prefix(expr->leftOperand);
        print_expression_prefix(expr->rightOperand);
    }
    else if (expr->tag == PRIMARY)
    {
        if (expr->primary->tag == ID)
        {
            //printf("%s ", expr->primary->id);
        }
        else if (expr->primary->tag == NUM)
        {
            //printf("%d ", expr->primary->ival);
        }
        else if (expr->primary->tag == REALNUM)
        {
            //printf("%.4f ", expr->primary->fval);
        }
    }
}

void print_while_stmt(struct while_stmtNode* while_stmt)
{
    //printf("WHILE ");
    print_condition(while_stmt->condition);
    print_body(while_stmt->body);
}

void print_do_stmt(struct while_stmtNode* do_stmt)
{
    // TODO: implement this for your own debugging purposes

    //printf("DO ");
    print_body(do_stmt->body);
    //printf("WHILE ");
    print_condition(do_stmt->condition);
    //printf("\n");
}

void print_condition(struct conditionNode* condition)
{

    if (condition->left_operand->tag == NUM){
        //printf("%d", condition->left_operand->ival);
    }

    else if(condition->left_operand->tag == REALNUM) {
        //printf("%.4f", condition->left_operand->fval);
    }

    else if(condition->left_operand->tag == ID) {
        //printf("%s", condition->left_operand->id);
    }

    if (condition->right_operand != NULL) {

        //printf("%s", reserved[condition->relop]);

        if (condition->right_operand->tag == NUM){
            //printf("%d", condition->right_operand->ival);
        }

        else if(condition->right_operand->tag == REALNUM) {
            //printf("%.4f", condition->right_operand->fval);
        }

        else if(condition->right_operand->tag == ID) {
            //printf("%s", condition->right_operand->id);
        }

    }

}

void print_case(struct caseNode* cas)
{
    // TODO: implement this for your own debugging purposes
}

void print_case_list(struct case_listNode* case_list)
{
    // TODO: implement this for your own debugging purposes
}

void print_switch_stmt(struct switch_stmtNode* switc)
{
    // TODO: implement this for your own debugging purposes
}

/* -------------------- PARSING AND BUILDING PARSE TREE -------------------- */

// Note that the following function is not
// called case because case is a keyword in C/C++
struct caseNode* cas()
{
    // TODO: implement this for EC
    return NULL;
}

struct case_listNode* case_list()
{
    // TODO: implement this for EC
    return NULL;
}

struct switch_stmtNode* switch_stmt()
{
    // TODO: implement this for EC
    return NULL;
}

struct while_stmtNode* do_stmt()
{
    struct while_stmtNode* doStatement;

    t_type = getToken();

    if (t_type == DO) {

        doStatement = ALLOC(struct while_stmtNode);

        doStatement->body = body();

        //printf("Body assigned\n");

        t_type = getToken();

        if (t_type == WHILE) {

            t_type = getToken();

            doStatement->condition = condition();

            // t_type = getToken();
        }
    }

    return doStatement;
}

struct primaryNode* primary()
{
    struct primaryNode* prmryNode;

    prmryNode = ALLOC(struct primaryNode);

    t_type = getToken();

    if (t_type == NUM) {

        prmryNode->tag = NUM;
        prmryNode->ival = atoi(token);
        prmryNode->fval = 0;
        prmryNode->id = NULL;
    }

    else if (t_type == REALNUM) {

        prmryNode->tag = REALNUM;
        prmryNode->ival = 0;
        prmryNode->fval = atof(token);
        prmryNode->id = NULL;
    }

    else if (t_type == ID) {

        prmryNode->tag = ID;
        prmryNode->ival = 0;
        prmryNode->fval = 0;
        prmryNode->id = strdup(token);
    }

    return prmryNode;
}

struct conditionNode* condition()
{
    struct conditionNode* condNde;

    if (t_type == NUM || t_type == ID || t_type == REALNUM) {

        condNde = ALLOC(struct conditionNode);

        if (condNde->left_operand == NULL) {

            ungetToken();
            condNde->left_operand = primary();
            t_type = getToken();

            if (t_type == GREATER || t_type == GTEQ || t_type == LESS || t_type == NOTEQUAL || t_type == LTEQ) {
                condNde->relop = t_type;
                t_type = getToken();

                if (t_type == NUM || t_type == ID || t_type == REALNUM) {

                    ungetToken();
                    condNde->right_operand = primary();
                    t_type = getToken();
                }
            }
        }
        
        else {

            condNde->right_operand = NULL;
        }
    }



    return condNde;
}

struct while_stmtNode* while_stmt()
{
    //Get the struct for the while statement
    struct while_stmtNode* whileStmt;

    //Analyze the next token
    t_type = getToken();

    //If the next token is a WHILE statement, then allocate memory for the size of the while node struct
    if (t_type == WHILE)
    {
        //Allocate proper memory
        whileStmt = ALLOC(struct while_stmtNode);

        t_type = getToken();

        //Set the condition portion of the while statement 
        whileStmt->condition = condition();

        if (t_type == LBRACE) {

            ungetToken();

            whileStmt->body = body();

        }
    }

    return whileStmt;
}

struct exprNode* factor()
{
    struct exprNode* facto;
    
    t_type = getToken();
    if (t_type == LPAREN)
    {
        facto = expr();
        t_type = getToken();
        if (t_type == RPAREN)
        {
            return facto;
        }
        else
        {
            syntax_error("factor. RPAREN expected");
        }
    }
    else if (t_type == NUM)
    {
        facto = ALLOC(struct exprNode);
        facto->primary = ALLOC(struct primaryNode);
        facto->tag = PRIMARY;
        facto->op = NOOP;
        facto->leftOperand = NULL;
        facto->rightOperand = NULL;
        facto->primary->tag = NUM;
        facto->primary->ival = atoi(token);
        return facto;
    }
    else if (t_type == REALNUM)
    {
        facto = ALLOC(struct exprNode);
        facto->primary = ALLOC(struct primaryNode);
        facto->tag = PRIMARY;
        facto->op = NOOP;
        facto->leftOperand = NULL;
        facto->rightOperand = NULL;
        facto->primary->tag = REALNUM;
        facto->primary->fval = atof(token);
        return facto;
    }
    else if (t_type == ID)
    {
        facto = ALLOC(struct exprNode);
        facto->primary = ALLOC(struct primaryNode);
        facto->tag = PRIMARY;
        facto->op = NOOP;
        facto->leftOperand = NULL;
        facto->rightOperand = NULL;
        facto->primary->tag = ID;
        facto->primary->id = strdup(token);
        return facto;
    }
    else
    {
        syntax_error("factor. NUM, REALNUM, or ID, expected");
    }
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct exprNode* term()
{
    struct exprNode* ter;
    struct exprNode* f;
    
    t_type = getToken();
    if (t_type == ID || t_type == LPAREN || t_type == NUM || t_type == REALNUM)
    {
        ungetToken();
        f = factor();
        t_type = getToken();
        if (t_type == MULT || t_type == DIV)
        {
            ter = ALLOC(struct exprNode);
            ter->op = t_type;
            ter->leftOperand = f;
            ter->rightOperand = term();
            ter->tag = EXPR;
            ter->primary = NULL;
            return ter;
        }
        else if (t_type == SEMICOLON || t_type == PLUS ||
                 t_type == MINUS || t_type == RPAREN)
        {
            ungetToken();
            return f;
        }
        else
        {
            syntax_error("term. MULT or DIV expected");
        }
    }
    else
    {
        syntax_error("term. ID, LPAREN, NUM, or REALNUM expected");
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct exprNode* expr()
{
    struct exprNode* exp;
    struct exprNode* t;
    
    t_type = getToken();
    if (t_type == ID || t_type == LPAREN || t_type == NUM || t_type == REALNUM)
    {
        ungetToken();
        t = term();
        t_type = getToken();
        if (t_type == PLUS || t_type == MINUS)
        {
            exp = ALLOC(struct exprNode);
            exp->op = t_type;
            exp->leftOperand = t;
            exp->rightOperand = expr();
            exp->tag = EXPR;
            exp->primary = NULL;
            return exp;
        }
        else if (t_type == SEMICOLON || t_type == MULT ||
                 t_type == DIV || t_type == RPAREN)
        {
            ungetToken();
            return t;
        }
        else
        {
            syntax_error("expr. PLUS, MINUS, or SEMICOLON expected");
        }
    }
    else
    {
        syntax_error("expr. ID, LPAREN, NUM, or REALNUM expected");
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct assign_stmtNode* assign_stmt()
{
    struct assign_stmtNode* assignStmt;
    
    t_type = getToken();
    if (t_type == ID)
    {
        assignStmt = ALLOC(struct assign_stmtNode);
        assignStmt->id = strdup(token);
        assignStmt->lineNumberTracker = line_no;
        t_type = getToken();
        if (t_type == EQUAL)
        {
            assignStmt->expr = expr();
            t_type = getToken();
            if (t_type == SEMICOLON)
            {
                return assignStmt;
            }
            else
            {
                syntax_error("asign_stmt. SEMICOLON expected");
            }
        }
        else
        {
            syntax_error("assign_stmt. EQUAL expected");
        }
    }
    else
    {
        syntax_error("assign_stmt. ID expected");
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct stmtNode* stmt()
{
    struct stmtNode* stm;
    
    stm = ALLOC(struct stmtNode);
    t_type = getToken();
    if (t_type == ID) // assign_stmt
    {
        ungetToken();
        stm->assign_stmt = assign_stmt();
        stm->stmtType = ASSIGN;
    }
    else if (t_type == WHILE) // while_stmt
    {
        ungetToken();
        stm->while_stmt = while_stmt();
        stm->stmtType = WHILE;
    }
    else if (t_type == DO)  // do_stmt
    {
        ungetToken();
        stm->while_stmt = do_stmt();
        stm->stmtType = DO;
    }
    else if (t_type == SWITCH) // switch_stmt
    {
        ungetToken();
        stm->switch_stmt = switch_stmt();
        stm->stmtType = SWITCH;
    }
    else
    {
        syntax_error("stmt. ID, WHILE, DO or SWITCH expected");
    }
    return stm;
}

struct stmt_listNode* stmt_list()
{
    struct stmt_listNode* stmtList;
    
    t_type = getToken();
    if (t_type == ID || t_type == WHILE ||
        t_type == DO || t_type == SWITCH)
    {
        ungetToken();
        stmtList = ALLOC(struct stmt_listNode);
        stmtList->stmt = stmt();
        t_type = getToken();
        if (t_type == ID || t_type == WHILE ||
            t_type == DO || t_type == SWITCH)
        {
            ungetToken();
            stmtList->stmt_list = stmt_list();
            return stmtList;
        }
        else // If the next token is not in FOLLOW(stmt_list),
            // let the caller handle it.
        {
            ungetToken();
            stmtList->stmt_list = NULL;
            return stmtList;
        }
    }
    else
    {
        syntax_error("stmt_list. ID, WHILE, DO or SWITCH expected");
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct bodyNode* body()
{
    struct bodyNode* bod;
    
    t_type = getToken();
    if (t_type == LBRACE)
    {
        bod = ALLOC(struct bodyNode);
        bod->stmt_list = stmt_list();
        t_type = getToken();
        if (t_type == RBRACE)
        {
            return bod;
        }
        else
        {
            syntax_error("body. RBRACE expected");
        }
    }
    else
    {
        syntax_error("body. LBRACE expected");
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct type_nameNode* type_name()
{
    struct type_nameNode* tName;
    
    tName = ALLOC(struct type_nameNode);
    t_type = getToken();
    if (t_type == ID || t_type == INT || t_type == REAL ||
        t_type == STRING || t_type == BOOLEAN || t_type == LONG)
    {
        tName->type = t_type;
        if (t_type == ID)
        {

            tName->id = strdup(token);
        }
        else
        {
            tName->id = NULL;
        }
        return tName;
    }
    else
    {
        syntax_error("type_name. type name expected");
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct id_listNode* id_list()
{
    struct id_listNode* idList;
    
    idList = ALLOC(struct id_listNode);
    t_type = getToken();
    if (t_type == ID)
    {
        idList->id = strdup(token);
        t_type = getToken();
        if (t_type == COMMA)
        {
            idList->id_list = id_list();
            return idList;
        }
        else if (t_type == COLON)
        {
            ungetToken();
            idList->id_list = NULL;
            return idList;
        }
        else
        {
            syntax_error("id_list. COMMA or COLON expected");
        }
    }
    else
    {
        syntax_error("id_list. ID expected");
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct type_declNode* type_decl()
{
    struct type_declNode* typeDecl;
    
    typeDecl = ALLOC(struct type_declNode);
    t_type = getToken();
    if (t_type == ID)
    {
        ungetToken();
        typeDecl->id_list = id_list();
        t_type = getToken();
        if (t_type == COLON)
        {
            typeDecl->type_name = type_name();
            t_type = getToken();
            if (t_type == SEMICOLON)
            {
                return typeDecl;
            }
            else
            {
                syntax_error("type_decl. SEMICOLON expected");
            }
        }
        else
        {
            syntax_error("type_decl. COLON expected");
        }
    }
    else
    {
        syntax_error("type_decl. ID expected");
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct var_declNode* var_decl()
{
    struct var_declNode* varDecl;

    //VERBOSE
    // switched_to_var = 1;
    
    varDecl = ALLOC(struct var_declNode);
    t_type = getToken();
    if (t_type == ID)
    {
        ungetToken();
        varDecl->id_list = id_list();
        t_type = getToken();
        if (t_type == COLON)
        {
            varDecl->type_name = type_name();
            t_type = getToken();
            if (t_type == SEMICOLON)
            {
                return varDecl;
            }
            else
            {
                syntax_error("var_decl. SEMICOLON expected");
            }
        }
        else
        {
            syntax_error("var_decl. COLON expected");
        }
    }
    else
    {
        syntax_error("var_decl. ID expected");
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct var_decl_listNode* var_decl_list()
{
    struct var_decl_listNode* varDeclList;
    
    varDeclList = ALLOC(struct var_decl_listNode);
    t_type = getToken();
    if (t_type == ID)
    {
        ungetToken();
        varDeclList->var_decl = var_decl();
        t_type = getToken();
        if (t_type == ID)
        {
            ungetToken();
            varDeclList->var_decl_list = var_decl_list();
            return varDeclList;
        }
        else
        {
            ungetToken();
            varDeclList->var_decl_list = NULL;
            return varDeclList;
        }
    }
    else
    {
        syntax_error("var_decl_list. ID expected");
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct type_decl_listNode* type_decl_list()
{
    struct type_decl_listNode* typeDeclList;
    
    typeDeclList = ALLOC(struct type_decl_listNode);
    t_type = getToken();
    if (t_type == ID)
    {
        ungetToken();
        typeDeclList->type_decl = type_decl();
        t_type = getToken();
        if (t_type == ID)
        {
            ungetToken();
            typeDeclList->type_decl_list = type_decl_list();
            return typeDeclList;
        }
        else
        {
            ungetToken();
            typeDeclList->type_decl_list = NULL;
            return typeDeclList;
        }
    }
    else
    {
        syntax_error("type_decl_list. ID expected");
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct var_decl_sectionNode* var_decl_section()
{
    struct var_decl_sectionNode *varDeclSection;
    
    varDeclSection = ALLOC(struct var_decl_sectionNode);
    t_type = getToken();
    if (t_type == VAR)
    {
        // no need to ungetToken()
        varDeclSection->var_decl_list = var_decl_list();
        return varDeclSection;
    }
    else
    {
        syntax_error("var_decl_section. VAR expected");
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct type_decl_sectionNode* type_decl_section()
{
    struct type_decl_sectionNode *typeDeclSection;
    
    typeDeclSection = ALLOC(struct type_decl_sectionNode);
    t_type = getToken();
    if (t_type == TYPE)
    {
        typeDeclSection->type_decl_list = type_decl_list();
        return typeDeclSection;
    }
    else
    {
        syntax_error("type_decl_section. TYPE expected");
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct declNode* decl()
{
    struct declNode* dec;
    
    dec = ALLOC(struct declNode);
    dec->type_decl_section = NULL;
    dec->var_decl_section = NULL;
    t_type = getToken();
    if (t_type == TYPE)
    {
        ungetToken();
        dec->type_decl_section = type_decl_section();
        t_type = getToken();
        if (t_type == VAR)
        {
            // type_decl_list is epsilon
            // or type_decl already parsed and the
            // next token is checked
            ungetToken();
            dec->var_decl_section = var_decl_section();
        }
        else
        {
            ungetToken();
            dec->var_decl_section = NULL;
        }
        return dec;
    }
    else
    {
        dec->type_decl_section = NULL;
        if (t_type == VAR)
        {
            // type_decl_list is epsilon
            // or type_decl already parsed and the
            // next token is checked
            ungetToken();
            dec->var_decl_section = var_decl_section();
            return dec;
        }
        else
        {
            if (t_type == LBRACE)
            {
                ungetToken();
                dec->var_decl_section = NULL;
                return dec;
            }
            else
            {
                syntax_error("decl. LBRACE expected");
            }
        }
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}

struct programNode* program()
{
    struct programNode* prog;
    
    prog = ALLOC(struct programNode);
    t_type = getToken();
    if (t_type == TYPE || t_type == VAR || t_type == LBRACE)
    {
        ungetToken();
        prog->decl = decl();
        prog->body = body();
        return prog;
    }
    else
    {
        syntax_error("program. TYPE or VAR or LBRACE expected");
    }
    assert(false);
    return NULL; // control never reaches here, this is just for the sake of GCC
}



//Error Code 0:
void check_duplicate_declarations() {

    int i, j;

    for (i = 0; i < type_id_count; i++) {

        for (j = i + 1; j < type_id_count; j++) {

            if (strcmp(type_ids[i], type_ids[j]) == 0) {

                printf("ERROR CODE 0 %s\n", type_ids[i]);
                error_found = 1;
                break;
            }
        }
    }
}

//Error Code 1:
void check_type_redec_var() {

    //This function needs to check to see if there is a multiple
    //definintion redeclaration of items in the type section
    //that are also existent as a left-hand side item in teh variable dec section

    int x, y;
    int size_of_type_list = type_id_count;
    int size_of_var_id_list = var_id_count;

    //If the type list is bigger than the var list, outer loop for type list
    if (size_of_type_list > size_of_var_id_list)
    {
        for (x = 0; x < size_of_type_list; x++)
        {
            for (y = 0; y < size_of_var_id_list; y++)
            {
                if (strcmp(type_ids[x], var_ids[y]) == 0)
                {
                    printf("ERROR CODE 1 %s", type_ids[x]);
                    error_found = 1;
                    break;
                }
            }

            if (error_found == 1)
            {
                break;
            }
        }
    }

    //If var list is bigger than type list, outer loop for var list

    else if (size_of_type_list < size_of_var_id_list)
    {
        for (x = 0; x < size_of_var_id_list; x++)
        {
            for (y = 0; y < size_of_type_list; y++)
            {
                if (strcmp(type_ids[y], var_ids[x]) == 0)
                {
                    printf("ERROR CODE 1 %s", type_ids[x]);
                    error_found = 1;
                    break;
                }
            }

            if (error_found == 1)
            {
                break;
            }
        }
    }

    //If both lists are equal, either or

    else if (size_of_type_list == size_of_var_id_list)
    {
        for (x = 0; x < size_of_var_id_list; x++)
        {
            for (y = 0; y < size_of_type_list; y++)
            {
                if (strcmp(type_ids[x], var_ids[y]) == 0)
                {
                    printf("ERROR CODE 1 %s", type_ids[x]);
                    error_found = 1;
                    break;
                }
            }

            if (error_found == 1)
            {
                break;
            }
        }
    }

    //ELSE compare first items

    else if (strcmp(type_ids[0], var_ids[0]) == 0)
    {
        printf("ERROR CODE 1 %s", type_ids[0]);
        error_found = 1;
        // break;
    }
}

//Error Code 2
void check_var_dec_multiple() {

    int i, j;

    for (i = 0; i < var_id_count; i++) {

        for (j = i + 1; j < var_id_count; j++) {

            if (strcmp(var_ids[i], var_ids[j]) == 0) {

                printf("ERROR CODE 2 %s", var_ids[i]);
                error_found = 1;
                break;
            }
        }
    }
}

//Error Code 4
void check_var_dec_as_type() {

    int x, y;

    // printf("\n\nINSIDE FUNCTION\n");

    //O(N^2) inefficient as fuck, should I sort and do binary search to make O(log(n))?
    if (var_id_count > var_assign_count) {

         // printf("\n\nFIRST\n");
        for (x = 0; x < var_id_count; x++)
        {
            for (y = 0; y < var_assign_count; y++)
            {
                // printf("Analyzing %s and %s\n", var_ids[x], var_assigns[y]);
                if (strcmp(var_ids[x], var_assigns[y]) == 0) {

                    // printf("They were identical\n");
                    printf("ERROR CODE 4 %s", (var_ids[x]));
                    error_found = 1;
                    break;
                }
            }

            if (error_found == 1){
                break;
            }
        }
    }

    else if (var_id_count < var_assign_count) {



        for (x = 0; x < var_assign_count; x++)
        {
            for (y = 0; y < var_id_count; y++)
            {
                // printf("Analyzing %s and %s\n", var_ids[x], var_assigns[y]);
                if (strcmp(var_ids[y], var_assigns[x]) == 0) {

                    // printf("They were identical\n");
                    printf("ERROR CODE 4 %s", (var_assigns[x]));
                    error_found = 1;
                    break;
                }
            }

            if (error_found == 1){
                break;
            }
        }
    }

    else if (var_id_count == var_assign_count) {

        // printf("\n\nTHIRD\n");

        for (x = 0; x < var_assign_count; x++)
        {
            for (y = 0; y < var_id_count; y++)
            {
                // printf("Analyzing %s and %s\n", var_ids[x], var_assigns[y]);
                if (strcmp(var_ids[x], var_assigns[y]) == 0) {

                    // printf("They were identical\n");
                    printf("ERROR CODE 4 %s", (var_ids[x]));
                    error_found = 1;
                    break;
                }
            }

            if (error_found == 1){
                break;
            }
        }
    }

    else if (strcmp(var_ids[0], var_assigns[0]) == 0) {

        printf("ERROR CODE 4 %s", (var_ids[0]));
        error_found = 1;

    }
}


int main()
{
    struct programNode* parseTree;

    parseTree = program();

    print_parse_tree(parseTree); // This is just for debugging purposes

    // Check Error Code 0:
    check_duplicate_declarations();

    //Check Error Code 1:
    if (error_found == 0) {check_type_redec_var();}

    //Check Error Code 2:
    if (error_found == 0) {check_var_dec_multiple();}

    // //Check Error Code 3:

    //[DO THIS DURING PARSING] ??

    // //Check Error Code 4:
    if (error_found == 0) {check_var_dec_as_type();}

    // //All Checks Passed Successfully!
    if (error_found == 0) {
        printf("All systems go!");
    }

    return 0;
}
