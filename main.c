#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_VARIABLES 100
#define MAX_STRING_LENGTH 100
#define MAX_IDENTIFIER_LENGTH 50
#define MAX_INTEGER_LENGTH 12
#define MAX_STACK_SIZE 100

typedef struct {
    char items[MAX_STACK_SIZE];
    int top;
} Stack;

void initializeStack(Stack *stack) {
    stack->top = -1;
}

int isStackEmpty(Stack *stack) {
    return stack->top == -1;
}

int isStackFull(Stack *stack) {
    return stack->top == MAX_STACK_SIZE - 1;
}

void push(Stack *stack, char value) {
    if (!isStackFull(stack)) {
        stack->items[++stack->top] = value;
    } else {
        fprintf(stderr, "Error: Stack overflow\n");
        exit(1);
    }
}

char pop(Stack *stack) {
    if (!isStackEmpty(stack)) {
        return stack->items[stack->top--];
    } else {
        fprintf(stderr, "Error: Stack underflow\n");
        exit(1);
    }
}

typedef enum {
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_OPERATOR,
    TOKEN_STRING,
    TOKEN_INTEGER,
    TOKEN_END_OF_LINE,
    TOKEN_COMMA,
    TOKEN_LEFT_CURLY_BRACKET,
    TOKEN_RIGHT_CURLY_BRACKET,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_ERROR
} TokenType;

// Represents a token in the code, with type and value
typedef struct {
    TokenType type;
    char value[MAX_STRING_LENGTH + 1];
} Token;

typedef enum {
    NODE_INT,
    NODE_STRING,
    NODE_VAR,
    NODE_ASSIGN,
    NODE_WRITE,
    NODE_READ,
    NODE_NEWLINE,
    NODE_LOOP,
    NODE_BLOCK,
    NODE_EXPRESSION
} NodeType;

typedef struct ASTNode {
    NodeType type;
    union {
        int intValue;
        char stringValue[MAX_STRING_LENGTH + 1];
        char varName[MAX_IDENTIFIER_LENGTH + 1];
        struct {
            struct ASTNode *left;
            struct ASTNode *right;
            char op;
        } assign;
        struct {
            struct ASTNode *condition;
            struct ASTNode *body;
        } loop;
        struct ASTNode *block;
    } data;
    struct ASTNode *next;
} ASTNode;

ASTNode* create_loop_node(ASTNode* condition, ASTNode* body) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    node->type = NODE_LOOP;
    node->data.loop.condition = condition;
    node->data.loop.body = body;
    node->next = NULL;
    return node;
}

ASTNode* create_int_node(int value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    node->type = NODE_INT;
    node->data.intValue = value;
    node->next = NULL;
    return node;
}

ASTNode* create_string_node(const char* value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    node->type = NODE_STRING;
    strcpy(node->data.stringValue, value);
    node->next = NULL;
    return node;
}

ASTNode* create_var_node(const char* name) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    node->type = NODE_VAR;
    strcpy(node->data.varName, name);
    node->next = NULL;
    return node;
}

ASTNode* create_assign_node(ASTNode* left, ASTNode* right, char op) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    node->type = NODE_ASSIGN;
    node->data.assign.left = left;
    node->data.assign.right = right;
    node->data.assign.op = op;
    node->next = NULL;
    return node;
}

ASTNode* create_block_node(ASTNode* statements) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    node->type = NODE_BLOCK;
    node->data.block = statements;
    node->next = NULL;
    return node;
}

typedef struct {
    int intValue;
    char stringValue[MAX_STRING_LENGTH + 1];
    int isInteger;
} Result;

typedef struct {
    char name[MAX_IDENTIFIER_LENGTH + 1];
    char value[MAX_STRING_LENGTH + 1];
    int isInteger;
} Variable;

typedef struct {
    Result result;
    Variable variables[MAX_VARIABLES];
    int variableCount;
    int loopDepth;
    int currentLine;
    const char* fileName;
    int maxLoopDepth;  // Maksimum döngü derinliği
    int errorCount;    // Toplam hata sayısı
    char lastErrorMessage[256];  // Son hata mesajı
    int loopCount;  // Loop sayısını takip etmek için ekledik
    // Diğer context bilgileri burada olabilir
} Context;

typedef struct {
    ASTNode* initial;
    ASTNode* condition;
    ASTNode* body;
    ASTNode* increment;
} ForLoopNode;

Variable variables[MAX_VARIABLES];
int variableCount = 0;

// Function prototypes
Token getNextToken(const char* line, int* index);
void set_variable(Context* context, const char* name, const char* value, int isInteger);
Variable* get_variable(Context* context, const char* name);
void execute_line(const char* line, Context* context);
int evaluate_expression(const char* expression, Context* context);
void handle_write(const char* line, int* index, Context* context);
void handle_loop(const char* line, int* index, Context* context);
int eval_condition(ASTNode* node, Context* context);
int eval_expression(ASTNode* node, Context* context);
void eval(ASTNode* node, Context* context);
int eval_expression_helper(const char** expr, Context* context);

const char operators[] = {'+', '-', '*', '/'};

// Global definitions for keywords and operators
const char* keywords[] = {
    "int", "text", "is", "loop", "times", "read", "write", "newLine"
};

// Checks if the given character is an operator
int is_operator(char ch) {
    return strchr(operators, ch) != NULL;
}

// Checks if the given identifier is a keyword
int is_keyword(const char* identifier) {
    for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strcmp(identifier, keywords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Function to remove comments and update is_comment_open flag
int strip_comments(char* line, int* is_comment_open) {
    char* comment_start = strstr(line, "/*");
    char* comment_end = strstr(line, "*/");

    // If a comment is already open and there is no closing, clear the line
    if (*is_comment_open) {
        if (comment_end) {
            memmove(line, comment_end + 2, strlen(comment_end + 2) + 1);
            *is_comment_open = 0;
        } else {
            line[0] = '\0';
            return 0;
        }
    }

    // Check new comment start and end in line
    while (comment_start) {
        if (comment_end && comment_end > comment_start) {
            memmove(comment_start, comment_end + 2, strlen(comment_end + 2) + 1);
            comment_start = strstr(comment_start, "/*");
        } else {
            *is_comment_open = 1;
            line[comment_start - line] = '\0';
            break;
        }
    }

    return 1;  // Comments are disabled or absent
}

// Handles identifier tokens, checking if they are keywords or regular identifiers
void handle_identifier(FILE* outputFile, const char* start, int length) {
    char identifier[MAX_IDENTIFIER_LENGTH + 1];
    strncpy(identifier, start, length);
    identifier[length] = '\0';

    if (!isalpha(identifier[0])) {
        fprintf(outputFile, "Error: Invalid identifier.\n");
    }
}

// Handles integer tokens
void handle_integer(FILE* outputFile, const char* start, int length) {
    char integer[MAX_INTEGER_LENGTH + 1];
    strncpy(integer, start, length);
    integer[length] = '\0';
}

// Handles string tokens, checks for unclosed or invalid strings
void handle_string(FILE* outputFile, const char* start, int length) {
    char string[MAX_STRING_LENGTH + 1];
    strncpy(string, start, length);
    string[length] = '\0';
}

// Adds or updates a variable
void set_variable(Context* context, const char* name, const char* value, int isInteger) {
    for (int i = 0; i < context->variableCount; i++) {
        if (strcmp(context->variables[i].name, name) == 0) {
            strcpy(context->variables[i].value, value);
            context->variables[i].isInteger = isInteger;
            return;
        }
    }

    if (context->variableCount < MAX_VARIABLES) {
        strcpy(context->variables[context->variableCount].name, name);
        strcpy(context->variables[context->variableCount].value, value);
        context->variables[context->variableCount].isInteger = isInteger;
        context->variableCount++;
    } else {
        fprintf(stderr, "Error: Too many variables defined\n");
        context->errorCount++;
        strcpy(context->lastErrorMessage, "Too many variables defined");
        exit(1);
    }
}

Variable* get_variable(Context* context, const char* name) {
    for (int i = 0; i < context->variableCount; i++) {
        if (strcmp(context->variables[i].name, name) == 0) {
            return &context->variables[i];
        }
    }
    return NULL;
}

// Function prototype for getNextToken
Token getNextToken(const char* line, int* index);

// Executes a single line of STAR code
void execute_line(const char* line, Context* context) {
    int index = 0;
    Token token = getNextToken(line, &index);

    if (token.type == TOKEN_KEYWORD && strcmp(token.value, "loop") == 0) {
        handle_loop(line, &index, context);
    } else if (token.type == TOKEN_KEYWORD && strcmp(token.value, "write") == 0) {
        handle_write(line, &index, context);
    }

    if (token.type == TOKEN_KEYWORD && (strcmp(token.value, "int") == 0 || strcmp(token.value, "text") == 0)) {
        char varType[6];
        strcpy(varType, token.value);

        while ((token = getNextToken(line, &index)).type != TOKEN_END_OF_LINE) {
            if (token.type == TOKEN_IDENTIFIER) {
                set_variable(context, token.value, varType[0] == 'i' ? "0" : "", varType[0] == 'i');
            } else if (token.type == TOKEN_COMMA) {
                continue;
            } else if (token.type == TOKEN_KEYWORD && strcmp(token.value, "is") == 0) {
                token = getNextToken(line, &index);
                if (token.type == TOKEN_INTEGER || token.type == TOKEN_STRING) {
                    set_variable(context, token.value, token.value, token.type == TOKEN_INTEGER);
                } else {
                    fprintf(stderr, "Error: Expected integer or string value after 'is', found '%s'\n", token.value);
                    context->errorCount++;
                    strcpy(context->lastErrorMessage, "Expected integer or string value after 'is'");
                    exit(1);
                }
            } else {
                fprintf(stderr, "Error: Expected identifier after 'int' or 'text'\n");
                context->errorCount++;
                strcpy(context->lastErrorMessage, "Expected identifier after 'int' or 'text'");
                exit(1);
            }
        }
        return;
    } else if (token.type == TOKEN_IDENTIFIER) {
        char varName[MAX_IDENTIFIER_LENGTH + 1] = {0};
        strcpy(varName, token.value);
        token = getNextToken(line, &index);
        if (token.type == TOKEN_KEYWORD && strcmp(token.value, "is") == 0) {
            char expr[MAX_STRING_LENGTH + 1] = {0};
            strncpy(expr, line + index, strlen(line) - index);
            int result = evaluate_expression(expr, context);
            char resultStr[MAX_INTEGER_LENGTH + 1];
            sprintf(resultStr, "%d", result);
            set_variable(context, varName, resultStr, 1);
        } else if (token.type == TOKEN_STRING) {
            set_variable(context, varName, token.value, 0);
        } else {
            fprintf(stderr, "Error: Expected 'is' or string after identifier\n");
            context->errorCount++;
            strcpy(context->lastErrorMessage, "Expected 'is' or string after identifier");
            exit(1);
        }
    } else if (token.type == TOKEN_KEYWORD && strcmp(token.value, "write") == 0) {
        handle_write(line, &index, context);
    } else if (token.type == TOKEN_KEYWORD && strcmp(token.value, "read") == 0) {
        token = getNextToken(line, &index);
        if (token.type == TOKEN_STRING) {
            printf("%s", token.value);
        }
        token = getNextToken(line, &index);
        if (token.type == TOKEN_IDENTIFIER) {
            char input[MAX_STRING_LENGTH + 1];
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = '\0';
            set_variable(context, token.value, input, 0);
        } else {
            fprintf(stderr, "Error: Expected identifier after 'read'\n");
            context->errorCount++;
            strcpy(context->lastErrorMessage, "Expected identifier after 'read'");
            exit(1);
        }
    } else if (token.type == TOKEN_KEYWORD && strcmp(token.value, "newLine") == 0) {
        printf("\n");
    } else {
        if (strlen(line) == 0 || strcmp(line, "\n") == 0) {
            return;
        }
        fprintf(stderr, "Error: Unrecognized statement '%s'\n", token.value);
        context->errorCount++;
        strcpy(context->lastErrorMessage, "Unrecognized statement");
        exit(1);
    }
}

int evaluate_expression(const char* expression, Context* context) {
    const char* expr = expression;
    return eval_expression_helper(&expr, context);
}

// Retrieves the next token from the line of code
Token getNextToken(const char* line, int* index) {
    Token token;
    token.type = TOKEN_ERROR;
    token.value[0] = '\0';

    while (isspace(line[*index])) (*index)++;

    if (line[*index] == '\0') {
        token.type = TOKEN_END_OF_LINE;
    } else if (isalpha(line[*index])) {
        int start = *index;
        while (isalnum(line[*index]) || line[*index] == '_') (*index)++;
        int length = *index - start;
        if (length > MAX_IDENTIFIER_LENGTH) {
            fprintf(stderr, "Error: Identifier too long.\n");
        } else {
            strncpy(token.value, line + start, length);
            token.value[length] = '\0';
            token.type = is_keyword(token.value) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
        }
    } else if (isdigit(line[*index])) {
        int start = *index;
        while (isdigit(line[*index])) (*index)++;
        int length = *index - start;
        if (length > MAX_INTEGER_LENGTH) {
            fprintf(stderr, "Error: Integer too long.\n");
        } else {
            strncpy(token.value, line + start, length);
            token.value[length] = '\0';
            token.type = TOKEN_INTEGER;
        }
    } else if (line[*index] == '"') {  // Handle string literals
        int start = ++(*index);
        int string_length = 0;
        while (line[*index] != '"' && line[*index] != '\0') {
            string_length++;
            if (string_length > MAX_STRING_LENGTH) {
                fprintf(stderr, "Error: String too long.\n");
                while (line[*index] != '"' && line[*index] != '\0') (*index)++;
                break;
            }
            (*index)++;
        }
        if (string_length <= MAX_STRING_LENGTH) {
            int length = *index - start;
            strncpy(token.value, line + start, length);
            token.value[length] = '\0';
            token.type = TOKEN_STRING;
        }
        if (line[*index] == '"') (*index)++;
    } else if (is_operator(line[*index])) {
        token.value[0] = line[*index];
        token.value[1] = '\0';
        token.type = TOKEN_OPERATOR;
        (*index)++;
    } else {
        switch (line[*index]) {
            case '.':
                token.type = TOKEN_END_OF_LINE;
                token.value[0] = '.';
                token.value[1] = '\0';
                (*index)++;
                break;
            case ',':
                token.type = TOKEN_COMMA;
                token.value[0] = ',';
                token.value[1] = '\0';
                (*index)++;
                break;
            case '{':
                token.type = TOKEN_LEFT_CURLY_BRACKET;
                token.value[0] = '{';
                token.value[1] = '\0';
                (*index)++;
                break;
            case '}':
                token.type = TOKEN_RIGHT_CURLY_BRACKET;
                token.value[0] = '}';
                token.value[1] = '\0';
                (*index)++;
                break;
            case '(':
                token.type = TOKEN_LEFT_PAREN;
                token.value[0] = '(';
                token.value[1] = '\0';
                (*index)++;
                break;
            case ')':
                token.type = TOKEN_RIGHT_PAREN;
                token.value[0] = ')';
                token.value[1] = '\0';
                (*index)++;
                break;
            default:
                fprintf(stderr, "Error: Unrecognized token '%c'.\n", line[*index]);
                token.type = TOKEN_ERROR;
                (*index)++;
                break;
        }
    }

    return token;
}

// Handles write statements
void handle_write(const char* line, int* index, Context* context) {
    Token token;
    while ((token = getNextToken(line, index)).type != TOKEN_END_OF_LINE) {
        if (token.type == TOKEN_IDENTIFIER) {
            Variable* var = get_variable(context, token.value);
            if (var) {
                printf("%s", var->value);
            } else {
                fprintf(stderr, "Error: Undefined variable '%s'\n", token.value);
                exit(1);
            }
        } else if (token.type == TOKEN_STRING || token.type == TOKEN_INTEGER) {
            printf("%s", token.value);
        } else if (token.type == TOKEN_COMMA) {
            printf(" ");
        }
    }
    printf("\n");
}

// Handles loop statements
void eval_loop(ASTNode* node, Context* context) {
    while (eval_condition(node->data.loop.condition, context)) {
        eval(node->data.loop.body, context);
    }
}

int eval_condition(ASTNode* node, Context* context) {
    if (node->type == NODE_INT) {
        return node->data.intValue != 0;
    }

    if (node->type == NODE_VAR) {
        Variable* var = get_variable(context, node->data.varName);
        if (var) {
            if (var->isInteger) {
                return atoi(var->value) != 0;
            } else {
                fprintf(stderr, "Error: Variable '%s' is not an integer\n", node->data.varName);
                context->errorCount++;
                strcpy(context->lastErrorMessage, "Variable is not an integer");
                exit(1);
            }
        } else {
            fprintf(stderr, "Error: Undefined variable '%s'\n", node->data.varName);
            context->errorCount++;
            strcpy(context->lastErrorMessage, "Undefined variable");
            exit(1);
        }
    }

    if (node->type == NODE_EXPRESSION) {
        int leftValue = eval_expression(node->data.assign.left, context);
        int rightValue = eval_expression(node->data.assign.right, context);
        switch (node->data.assign.op) {
            case '>':
                return leftValue > rightValue;
            case '<':
                return leftValue < rightValue;
            case '>=':
                return leftValue >= rightValue;
            case '<=':
                return leftValue <= rightValue;
            case '==':
                return leftValue == rightValue;
            case '!=':
                return leftValue != rightValue;
            default:
                fprintf(stderr, "Error: Unknown operator '%c' in condition\n", node->data.assign.op);
                context->errorCount++;
                strcpy(context->lastErrorMessage, "Unknown operator in condition");
                exit(1);
        }
    }

    fprintf(stderr, "Error: Invalid node type in condition\n");
    context->errorCount++;
    strcpy(context->lastErrorMessage, "Invalid node type in condition");
    exit(1);
}

int eval_expression_helper(const char** expr, Context* context) {
    int result = 0;
    int sign = 1;
    int current_value = 0;
    int operand_set = 0;
    char current_operator = '+';

    while (**expr) {
        if (isspace(**expr)) {
            (*expr)++;
            continue;
        }

        if (isdigit(**expr)) {
            current_value = 0;
            while (isdigit(**expr)) {
                current_value = current_value * 10 + (**expr - '0');
                (*expr)++;
            }
            operand_set = 1;
        } else if (isalpha(**expr)) {
            char varName[MAX_IDENTIFIER_LENGTH + 1] = {0};
            int len = 0;
            while (isalnum(**expr) || **expr == '_') {
                varName[len++] = **expr;
                (*expr)++;
            }
            varName[len] = '\0';
            Variable* var = get_variable(context, varName);
            if (var) {
                current_value = atoi(var->value);
                operand_set = 1;
            } else {
                fprintf(stderr, "Error: Undefined variable '%s'\n", varName);
                exit(1);
            }
        } else if (**expr == '(') {
            (*expr)++;
            current_value = eval_expression_helper(expr, context);
            operand_set = 1;
        } else if (**expr == ')') {
            (*expr)++;
            break;
        } else if (is_operator(**expr)) {
            if (!operand_set) {
                fprintf(stderr, "Error: Expected operand before operator '%c'\n", **expr);
                exit(1);
            }
            if (current_operator == '+') {
                result += sign * current_value;
            } else if (current_operator == '-') {
                result -= sign * current_value;
            } else if (current_operator == '*') {
                result *= current_value;
            } else if (current_operator == '/') {
                result /= current_value;
            }
            current_operator = **expr;
            sign = (current_operator == '-') ? -1 : 1;
            (*expr)++;
            operand_set = 0;
        } else if (**expr == '&' && *(*expr + 1) == '&') {
            (*expr) += 2;
            int rhs = eval_expression_helper(expr, context);
            return (result != 0) && (rhs != 0);
        } else if (**expr == '|' && *(*expr + 1) == '|') {
            (*expr) += 2;
            int rhs = eval_expression_helper(expr, context);
            return (result != 0) || (rhs != 0);
        } else if (**expr == '.') {
            (*expr)++;
            break;
        } else {
            fprintf(stderr, "Error: Unexpected character '%c' in expression\n", **expr);
            exit(1);
        }
    }

    if (operand_set) {
        if (current_operator == '+') {
            result += sign * current_value;
        } else if (current_operator == '-') {
            result -= sign * current_value;
        } else if (current_operator == '*') {
            result *= current_value;
        } else if (current_operator == '/') {
            result /= current_value;
        }
    }

    return result;
}

void eval(ASTNode* node, Context* context) {
    switch (node->type) {
        case NODE_INT:
            context->result.intValue = node->data.intValue;
            context->result.isInteger = 1;
            break;
        case NODE_STRING:
            strcpy(context->result.stringValue, node->data.stringValue);
            context->result.isInteger = 0;
            break;
        case NODE_VAR: {
            Variable* var = get_variable(context, node->data.varName);
            if (var) {
                if (var->isInteger) {
                    context->result.intValue = atoi(var->value);
                    context->result.isInteger = 1;
                } else {
                    strcpy(context->result.stringValue, var->value);
                    context->result.isInteger = 0;
                }
            } else {
                fprintf(stderr, "Error: Undefined variable '%s'\n", node->data.varName);
                context->errorCount++;
                strcpy(context->lastErrorMessage, "Undefined variable");
                exit(1);
            }
            break;
        }
        case NODE_ASSIGN:
            eval(node->data.assign.right, context);
            if (context->result.isInteger) {
                char resultStr[MAX_INTEGER_LENGTH + 1];
                sprintf(resultStr, "%d", context->result.intValue);
                set_variable(context, node->data.assign.left->data.varName, resultStr, 1);
            } else {
                set_variable(context, node->data.assign.left->data.varName, context->result.stringValue, 0);
            }
            break;
        case NODE_WRITE:
            eval(node->data.assign.right, context);
            if (context->result.isInteger) {
                printf("%d", context->result.intValue);
            } else {
                printf("%s", context->result.stringValue);
            }
            break;
        case NODE_READ: {
            char input[MAX_STRING_LENGTH + 1];
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = '\0';
            set_variable(context, node->data.varName, input, 0);
            break;
        }
        case NODE_NEWLINE:
            printf("\n");
            break;
        case NODE_LOOP:
            eval_loop(node, context);
            break;
        case NODE_BLOCK: {
            ASTNode* stmt = node->data.block;
            while (stmt) {
                eval(stmt, context);
                stmt = stmt->next;
            }
            break;
        }
        case NODE_EXPRESSION:
            context->result.intValue = eval_expression(node, context);
            context->result.isInteger = 1;
            break;
        default:
            fprintf(stderr, "Error: Unknown node type %d\n", node->type);
            context->errorCount++;
            strcpy(context->lastErrorMessage, "Unknown node type");
            exit(1);
    }
}

int eval_expression(ASTNode* node, Context* context) {
    switch (node->type) {
        case NODE_INT:
            return node->data.intValue;
        case NODE_VAR: {
            Variable* var = get_variable(context, node->data.varName);
            if (var && var->isInteger) {
                return atoi(var->value);
            } else {
                fprintf(stderr, "Error: Variable '%s' is not an integer\n", node->data.varName);
                context->errorCount++;
                strcpy(context->lastErrorMessage, "Variable is not an integer");
                exit(1);
            }
        }
        case NODE_EXPRESSION: {
            int left = eval_expression(node->data.assign.left, context);
            int right = eval_expression(node->data.assign.right, context);
            switch (node->data.assign.op) {
                case '+':
                    return left + right;
                case '-':
                    return left - right;
                case '*':
                    return left * right;
                case '/':
                    if (right == 0) {
                        fprintf(stderr, "Error: Division by zero\n");
                        context->errorCount++;
                        strcpy(context->lastErrorMessage, "Division by zero");
                        exit(1);
                    }
                    return left / right;
                default:
                    fprintf(stderr, "Error: Unknown operator '%c'\n", node->data.assign.op);
                    context->errorCount++;
                    strcpy(context->lastErrorMessage, "Unknown operator in expression");
                    exit(1);
            }
        }
        default:
            fprintf(stderr, "Error: Unknown expression type %d\n", node->type);
            context->errorCount++;
            strcpy(context->lastErrorMessage, "Unknown expression type");
            exit(1);
    }
}

void process_line(const char* line, FILE* outputFile, int* is_comment_open, int* open_curly_brackets) {


    int i = 0;
    int len = strlen(line);

    if (*is_comment_open) { // using flag for comments
        char* comment_end = strstr(line, "*/");
        if (comment_end) {
            *is_comment_open = 0;  // Comment closed
            i = comment_end - line + 2;  // Continue processing
        } else {
            return;  // Comment is still open
        }
    }

    while (i < len) {
        while (isspace(line[i])) i++; // Skip whitespace
        if (line[i] == '\0') continue;  // If at the end of the line

        // Handle potential negative integers
        if (line[i] == '-' && isdigit(line[i + 1])) {
            fprintf(outputFile, "Error: Negative integer.\n");
            i++;
            while (isdigit(line[i])) i++; // Skip the rest of the integer
            continue;
        }

        // Handle operator tokens
        if (is_operator(line[i])) {
            i++;
            continue;
        }

        // Handle various other token types
        switch (line[i]) {
            case '.':  // End-of-line token
                i++;
                break;
            case ',': // Comma token
                i++;
                break;
            case '{': // Left curly bracket token
                (*open_curly_brackets)++;
                i++;
                break;
            case '}': // Right curly bracket token
                if (*open_curly_brackets > 0) {
                    (*open_curly_brackets)--;
                } else {
                    fprintf(outputFile, "Error: Unmatched right curly bracket.\n");
                }
                i++;
                break;
            case '(': // Left parenthesis token
            case ')': // Right parenthesis token
                i++;
                break;
            default:
                // Handle identifiers and keywords
                if (isalpha(line[i])) {
                    int start = i;
                    int length = 0;
                    while (isalnum(line[i]) || line[i] == '_') {
                        length++;
                        if (length > MAX_IDENTIFIER_LENGTH) {
                            fprintf(outputFile, "Error: Identifier too long.\n");
                            while (isalnum(line[i]) || line[i] == '_') {  // Skip the long identifier
                                i++;
                            }
                            break;
                        }
                        i++;
                    }
                    if (length <= MAX_IDENTIFIER_LENGTH) {
                        handle_identifier(outputFile, line + start, length);
                    }
                    continue;
                }

                // Handle integer tokens
                if (isdigit(line[i])) {
                    int start = i;
                    int length = 0;
                    while (isdigit(line[i])) {
                        length++;
                        if (length > MAX_INTEGER_LENGTH) {
                            fprintf(outputFile, "Error: Integer too long.\n");
                            while (isdigit(line[i])) i++;  // Skip the long integer
                            break;
                        }
                        i++;
                    }
                    if (length <= MAX_INTEGER_LENGTH) {
                        handle_integer(outputFile, line + start, length);
                    }
                    continue;
                }

                // Handle string tokens, check for unclosed or invalid strings
                if (line[i] == '"') {
                    int start = i;
                    int length = 1;
                    i++;
                    int string_too_long = 0;

                    while (line[i] != '"' && i < len) {
                        length++;
                        if (length > MAX_STRING_LENGTH) {
                            string_too_long = 1;
                            break;
                        }
                        i++;
                    }

                    if (string_too_long) {
                        fprintf(outputFile, "Error: String too long.\n");
                        while (line[i] != '\0' && line[i] != '"') i++; // Skip until end of the string or line
                        if (line[i] == '"') i++; // Skip the closing quote if present
                        continue;
                    }

                    if (line[i] == '"') {
                        length++;  // Include the closing quote

                        // Check if the string contains a double quote inside it
                        int isInvalidString = 0;

                        handle_string(outputFile, line + start, length); // Handle the valid string

                        for (int j = i + 1; j < strlen(line); j++) {
                            if (line[j] == '"') {
                                isInvalidString = 1;
                                break;
                            }
                            i = j + 2; // Move past the string
                        }

                        if (isInvalidString) {
                            fprintf(outputFile, "Error: String contains double quotes.\n");
                            continue;
                        }

                    } else {
                        fprintf(outputFile, "Error: Unclosed string.\n"); // If the string isn't closed
                    }
                    continue;
                }

                fprintf(outputFile, "Error: Unrecognized token.\n"); // If none of the conditions were met
                i++;
                break;
        }
    }
}

int handleCurlyBrackets(const char *line, int *index, Stack *stack) {
    while (line[*index] != '\0') {
        if (line[*index] == '{') {
            push(stack, '{');
        } else if (line[*index] == '}') {
            if (isStackEmpty(stack)) {
                return 0; // Mismatched closing bracket
            }
            pop(stack);
        }
        (*index)++;
    }
    return isStackEmpty(stack); // 1 if matched, 0 if unmatched
}

void handle_loop(const char *line, int *index, Context *context) {
    Token token = getNextToken(line, index);
    if (token.type != TOKEN_INTEGER) {
        fprintf(stderr, "Error: Expected integer for loop count\n");
        exit(1);
    }

    context->loopCount = atoi(token.value);  // Burada loopCount değişkenini kullanıyoruz
    int bodyStart = *index;
    int openBrackets = 0;
    int isCommentOpen = 0;
    int bodyEnd = bodyStart;

    while (line[bodyEnd] != '\0') {
        if (line[bodyEnd] == '/' && line[bodyEnd + 1] == '*') {
            isCommentOpen = 1;
            bodyEnd += 2;
            continue;
        }
        if (line[bodyEnd] == '*' && line[bodyEnd + 1] == '/') {
            isCommentOpen = 0;
            bodyEnd += 2;
            continue;
        }

        if (!isCommentOpen) {
            if (line[bodyEnd] == '{') {
                openBrackets++;
            } else if (line[bodyEnd] == '}') {
                openBrackets--;
                if (openBrackets == 0) {
                    break;
                }
            }
        }
        bodyEnd++;
    }

    if (openBrackets != 0) {
        fprintf(stderr, "Error: Mismatched curly brackets in loop body\n");
        exit(1);
    }

    char loopBody[1024];
    strncpy(loopBody, line + bodyStart, bodyEnd - bodyStart);
    loopBody[bodyEnd - bodyStart] = '\0';

    for (int i = 0; i < context->loopCount; i++) {
        int bodyIndex = 0;
        while (bodyIndex < strlen(loopBody)) {
            char lineBuffer[1024];
            int lineEnd = 0;
            while (loopBody[bodyIndex + lineEnd] != '\n' && loopBody[bodyIndex + lineEnd] != '\0') {
                lineEnd++;
            }
            strncpy(lineBuffer, loopBody + bodyIndex, lineEnd);
            lineBuffer[lineEnd] = '\0';
            execute_line(lineBuffer, context);
            bodyIndex += lineEnd + 1;
        }
    }
}

// Çok satırlı yorumlar için kontrol
void process_multiline_comments_and_brackets(const char* line, int* index, int* isCommentOpen, int* openBrackets) {
    while (line[*index] != '\0') {
        if (line[*index] == '/' && line[*index + 1] == '*') {
            *isCommentOpen = 1;
            *index += 2;
            continue;
        }
        if (line[*index] == '*' && line[*index + 1] == '/') {
            *isCommentOpen = 0;
            *index += 2;
            continue;
        }

        if (!*isCommentOpen) {
            if (line[*index] == '{') {
                (*openBrackets)++;
            } else if (line[*index] == '}') {
                (*openBrackets)--;
                if (*openBrackets == 0) {
                    break;
                }
            }
        }
        (*index)++;
    }
}

void lexicalAnalyzer(const char* inputFilePath, const char* outputFilePath) {
    FILE* inputFile = fopen(inputFilePath, "r");
    if (!inputFile) {
        fprintf(stderr, "Error: Could not open input file.\n");
        return;
    }

    FILE* outputFile = fopen(outputFilePath, "w");
    if (!outputFile) {
        fclose(inputFile);
        fprintf(stderr, "Error: Could not open output file.\n");
        return;
    }

    int is_comment_open = 0;  // Flag for comments
    int open_curly_brackets = 0;  // Flag for curly brackets
    char line[1024];
    while (fgets(line, sizeof(line), inputFile)) {
        if (!strip_comments(line, &is_comment_open)) {  // If the comment is not closed, do not continue
            continue;  // If the comment is not closed, check the next line
        }

        // If comment is closed, commit current line
        if (!is_comment_open) {
            process_line(line, outputFile, &is_comment_open, &open_curly_brackets);
        }
    }

    // If there is still an open comment at the end of the file, print the error message
    if (is_comment_open) {
        fprintf(outputFile, "Error: Unclosed comment.\n");
    }

    fclose(inputFile);
    fclose(outputFile);
}

// Main interpreter function, reads and executes the code
void interpreter(const char* inputFilePath) {
    FILE* inputFile = fopen(inputFilePath, "r");
    if (!inputFile) {
        fprintf(stderr, "Error: Could not open input file.\n");
        return;
    }

    Context context = {0};
    context.fileName = inputFilePath;

    char line[1024];
    int is_comment_open = 0;
    int open_curly_brackets = 0;

    while (fgets(line, sizeof(line), inputFile)) {
        context.currentLine++;
        if (!strip_comments(line, &is_comment_open)) {
            continue;
        }
        if (!is_comment_open) {
            execute_line(line, &context);
        }
    }

    fclose(inputFile);

    if (context.errorCount > 0) {
        fprintf(stderr, "Total errors: %d\n", context.errorCount);
        fprintf(stderr, "Last error: %s\n", context.lastErrorMessage);
    }
}

int main() {
    interpreter("code.sta");
    return 0;
}
