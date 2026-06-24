#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define EPS 1e-9

// Types of nodes in operations tree
typedef enum node_type
{
    X, CONST, E, PI, SIN, COS, TAN, CTG, SUM, DIF, MUL, DIV
} node_type;

// a single node in operations tree. Used for all types of nodes
typedef struct tree_node
{
    node_type type;
    double const_value; // being used if node type is CONST
    struct tree_node* left;
    struct tree_node* right;
} tree_node;


void* safe_malloc(size_t size);

tree_node* make_node(void);

void free_node_recursively(tree_node* node);

tree_node* deep_copy_node(tree_node* node);

tree_node* parse_function(char* function);

tree_node* calculate_derivative_tree(tree_node* node);

void write_node_to_file_recursively(FILE* file, tree_node* node);

void write_function_tree_to_file(FILE* file, char* func_name, tree_node* head);


// This will be used to count consts inside .asm file
static unsigned const_counter;

int main(void)
{
    // char* input_file_path = "in.txt"; // for debugging
    char* input_file_path = getenv("SPEC_FILE");
    if (input_file_path == NULL)
    {
        printf("SPEC_FILE is not set\n");
        return -1;
    }

    FILE* input_file = fopen(input_file_path, "r");
    FILE* output_file = fopen("functions-custom.asm", "w");

    const_counter = 0;
	
    char buffer[200];
    if (fgets(buffer, sizeof(buffer), input_file) == NULL) // skipping the first line
    {
	printf("Error reading the first line of %s\n", input_file_path);
	fclose(input_file);
	fclose(output_file);
	return -1;
    }

    for (unsigned i = 0; i < 3; ++i)
    {
        if (fgets(buffer, sizeof(buffer), input_file) != NULL)
        {
            buffer[strcspn(buffer, "\n")] = '\0'; // removing "\n" at the end
            tree_node* function_tree = parse_function(buffer);
            tree_node* func_derivative = calculate_derivative_tree(function_tree);

            char fname[3];
            char dfname[4];
            snprintf(fname, sizeof(fname), "f%u", i+1);
            snprintf(dfname, sizeof(dfname), "df%u", i+1);

            write_function_tree_to_file(output_file, fname, function_tree);
            write_function_tree_to_file(output_file, dfname, func_derivative);

            free_node_recursively(function_tree);
            free_node_recursively(func_derivative);
        }
    }

    fclose(output_file);
    fclose(input_file);
    return 0;
} // int main(void)


// malloc that will check is memory allocation successful
void* safe_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr) return ptr;

    printf("Memory allocation unsuccessful\n");
    exit(1);
}

// creates a new node with default values. Makes node creating more safe
tree_node* make_node(void)
{
    tree_node* output = safe_malloc(sizeof(tree_node));
    output->const_value = 0.0;
    output->left = NULL;
    output->right = NULL;

    return output;
}

void free_node_recursively(tree_node* node)
{
    if (node == NULL) return;
    free_node_recursively(node->left);
    free_node_recursively(node->right);
    free(node);
}

tree_node* deep_copy_node(tree_node* node)
{
    if (node == NULL) return NULL;

    tree_node* copy = safe_malloc(sizeof(tree_node));
    copy->type = node->type;
    copy->const_value = node->const_value;

    copy->left = deep_copy_node(node->left);
    copy->right = deep_copy_node(node->right);

    return copy;
}

// constructs operations tree from polish notation funcition line
tree_node* parse_function(char* function)
{
    // I will be allocating memory for each node
    // and make a linked list to use as a stack
    typedef struct linked_node_list
    {
        tree_node* node;
        struct linked_node_list* previous;
    } linked_node_list;

    linked_node_list* unused_nodes_stack = 0;

    char* token = strtok(function, " ");
    while (token != NULL)
    {
        tree_node* new_node = make_node();

        char* end;
        double value = strtod(token, &end); // used to check if a number is a const

        // If node requires children, it will get 1 or 2 from unused_nodes_stack
        int arguments_amount = 0;

        if (strcmp(token, "x") == 0)
            new_node->type = X;
        else if (strcmp(token, "e") == 0)
            new_node->type = E;
        else if (strcmp(token, "pi") == 0)
            new_node->type = PI;
        else if (*end == '\0')
        {
            new_node->type = CONST;
            new_node->const_value = value;
        }
        else if (strcmp(token, "sin") == 0)
        {
            new_node->type = SIN;
            arguments_amount = 1;
        }
        else if (strcmp(token, "cos") == 0)
        {
            new_node->type = COS;
            arguments_amount = 1;
        }
        else if (strcmp(token, "tan") == 0)
        {
            new_node->type = TAN;
            arguments_amount = 1;
        }
        else if (strcmp(token, "ctg") == 0)
        {
            new_node->type = CTG;
            arguments_amount = 1;
        }
        else if (strcmp(token, "+") == 0)
        {
            new_node->type = SUM;
            arguments_amount = 2;
        }
        else if (strcmp(token, "-") == 0)
        {
            new_node->type = DIF;
            arguments_amount = 2;
        }
        else if (strcmp(token, "*") == 0)
        {
            new_node->type = MUL;
            arguments_amount = 2;
        }
        else if (strcmp(token, "/") == 0)
        {
            new_node->type = DIV;
            arguments_amount = 2;
        }
        else
        {
            printf("Parsing function failed: unknown argument {%s}\n", token);
            exit(1);
        }

        // filling each argument
        for (int arg_idx = 1; arg_idx <= arguments_amount; ++arg_idx)
        {
            if (unused_nodes_stack == 0)
            {
                printf("Parsing function failed: there was not enough argument for a function\n");
                exit(1);
            }

            if (arg_idx == 1) new_node->right = unused_nodes_stack->node;
            else if (arg_idx == 2) new_node->left = unused_nodes_stack->node;

            // removing argument from stack
            linked_node_list* new_stack_top = unused_nodes_stack->previous;
            free(unused_nodes_stack);
            unused_nodes_stack = new_stack_top;
        }

        // adding new node to the top of the stack
        linked_node_list* new_unused_stack_top = safe_malloc(sizeof(linked_node_list));
        new_unused_stack_top->node = new_node;
        new_unused_stack_top->previous = unused_nodes_stack;

        unused_nodes_stack = new_unused_stack_top;

        token = strtok(NULL, " "); // next token
    } // while (1)

    // by now the stack should have only one element – the tree itself
    if (unused_nodes_stack == 0 || unused_nodes_stack->previous != 0)
    {
        printf("Parsing function failed: stack size is not 1\n");
        exit(1);
    }

    tree_node* output = unused_nodes_stack->node;
    free(unused_nodes_stack);

    return output;
} // tree_node* parse_function(char* function)

tree_node* calculate_derivative_tree(tree_node* node)
{
    tree_node* output = make_node();

    // X, CONST, E, PI, SIN, COS, TAN, CTG, SUM, DIF, MUL, DIV
    switch (node->type)
    {
    case X: // x' = 1
        output->type = CONST;
        output->const_value = 1;
        break;
    case CONST: // a' = 0
    case E:
    case PI:
        output->type = CONST;
        output->const_value = 0;
        break;
    case SIN: // sin(a)' = a' * cos(a)
        output->type = MUL;
        output->left = calculate_derivative_tree(node->right);

        tree_node* cos = make_node();
        cos->type = COS;
        cos->right = deep_copy_node(node->right);
        output->right = cos;
        break;
    case COS: // cos(a)' = -1 * (a'* sin(a))
        {
            tree_node* sin_mul = make_node();
            sin_mul->type = MUL;
            sin_mul->left = calculate_derivative_tree(node->right);

            tree_node* sin = make_node();
            sin->type = SIN;
            sin->right = deep_copy_node(node->right);
            sin_mul->right = sin;

            output->type = MUL;
            output->left = make_node();
            output->left->type = CONST;
            output->left->const_value = -1.0;
            output->right = sin_mul;
            break;
        }
    case TAN: // tan(a)' = a' / cos(a)^2
        {
            tree_node* other_cos = make_node();
            other_cos->type = COS;
            other_cos->right = deep_copy_node(node->right);

            tree_node* cos_squared = make_node();
            cos_squared->type = MUL;
            cos_squared->left = other_cos;
            cos_squared->right = deep_copy_node(other_cos);

            output->type = DIV;
            output->left = calculate_derivative_tree(node->right);
            output->right = cos_squared;
            break;
        }
    case CTG: // ctg(a)' = -1 * a' / cos(a)^2
        {
            tree_node* other_sin = make_node();
            other_sin->type = SIN;
            other_sin->right = deep_copy_node(node->right);

            tree_node* sin_squared = make_node();
            sin_squared->type = MUL;
            sin_squared->left = other_sin;
            sin_squared->right = deep_copy_node(other_sin);

            tree_node* neg_derivative = make_node();
            neg_derivative->type = MUL;
            neg_derivative->left = make_node();
            neg_derivative->left->type = CONST;
            neg_derivative->left->const_value = -1.0;
            neg_derivative->right = calculate_derivative_tree(node->right);

            output->type = DIV;
            output->left = neg_derivative;
            output->right = sin_squared;
            break;
        }
    case SUM: // (a+b)' = a' + b'
        output->type = SUM;
        output->left = calculate_derivative_tree(node->left);
        output->right = calculate_derivative_tree(node->right);
        break;
    case DIF: // (a-b)' = a' - b'
        output->type = DIF;
        output->left = calculate_derivative_tree(node->left);
        output->right = calculate_derivative_tree(node->right);
        break;
    case MUL: // (a*b)' = a'b + b'a
        {
            tree_node* left_mul = make_node();
            left_mul->type = MUL;
            left_mul->left = calculate_derivative_tree(node->left);
            left_mul->right = deep_copy_node(node->right);

            tree_node* right_mul = make_node();
            right_mul->type = MUL;
            right_mul->left = calculate_derivative_tree(node->right);
            right_mul->right = deep_copy_node(node->left);

            output->type = SUM;
            output->left = left_mul;
            output->right = right_mul;
            break;
        }
    case DIV: // (a/b)' = (a'b - b'a) / b^2
        {
            tree_node* left_div = make_node();
            left_div->type = MUL;
            left_div->left = calculate_derivative_tree(node->left);
            left_div->right = deep_copy_node(node->right);

            tree_node* right_div = make_node();
            right_div->type = MUL;
            right_div->left = calculate_derivative_tree(node->right);
            right_div->right = deep_copy_node(node->left);

            tree_node* div_diff = make_node();
            div_diff->type = DIF;
            div_diff->left = left_div;
            div_diff->right = right_div;

            tree_node* right_squared = make_node();
            right_squared->type = MUL;
            right_squared->left = deep_copy_node(node->right);
            right_squared->right = deep_copy_node(node->right);

            output->type = DIV;
            output->left = div_diff;
            output->right = right_squared;
            break;
        }
    default:
            printf("Calculate derivative tree: Unknown node type\n");
            exit(1);
    } // switch (head->type)

    return output;
} // tree_node* calculate_derivative_tree(tree_node* head)

// the file should be opened with "w" mode
void write_node_to_file_recursively(FILE* file, tree_node* node)
{
    if (node == NULL) return;

    write_node_to_file_recursively(file, node->left);
    write_node_to_file_recursively(file, node->right);

    switch (node->type)
    {
    case X:
        fprintf(file, "\tfld qword[ebp+8]\n");
        break;
    case CONST:
        if (fabs(node->const_value - 1.0) < EPS)
            fprintf(file, "\tfld1\n");
        else if (fabs(node->const_value) < EPS)
            fprintf(file, "\tfldz\n");
        else
        {
            fprintf(file, "section .data\n");
            fprintf(file, "\tconst_%u dq %lf\n", const_counter, node->const_value);
            fprintf(file, "section .text\n");
            fprintf(file, "\tfld qword[const_%d]\n", const_counter);
            const_counter++;
        }
        break;
    case E:
        fprintf(file, "section .data\n");
        fprintf(file, "\tconst_%u dq 2.7182818\n", const_counter);
        fprintf(file, "section .text\n");
        fprintf(file, "\tfld qword[const_%d]\n", const_counter);
        const_counter++;
        break;
    case PI:
        fprintf(file, "\tfldpi\n");
        break;
    case SIN:
        fprintf(file, "\tfsin\n");
        break;
    case COS:
        fprintf(file, "\tfcos\n");
        break;
    case TAN:
        fprintf(file, "\tfptan\n");
        fprintf(file, "\tfstp st0\n");
        break;
    case CTG:
        fprintf(file, "\tfptan\n");
        fprintf(file, "\tfdivp st1, st0\n");
        break;
    case SUM:
        fprintf(file, "\tfaddp\n");
        break;
    case DIF:
        fprintf(file, "\tfsubp\n");
        break;
    case MUL:
        fprintf(file, "\tfmulp\n");
        break;
    case DIV:
        fprintf(file, "\tfdivp\n");
        break;
    default:
        break;
    } // switch (node->type)
} // void write_node_to_file_recursively(FILE* file, tree_node* node)

void write_function_tree_to_file(FILE* file, char* func_name, tree_node* head)
{
    fprintf(file, "section .text\nglobal %s\n%s:\n", func_name, func_name);
    fprintf(file, "\tpush ebp\n\tmov ebp, esp\n\tfinit\n");

    write_node_to_file_recursively(file, head);

    fprintf(file, "\tmov esp, ebp\n\tpop ebp\n\tret\n\n");
} // void write_function_tree_to_file(FILE* file, char* func_name, tree_node* head)
