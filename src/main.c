#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
  char *name;
  struct Node *children;
  int n_children;
} Node;

typedef struct Tree {
  Node *root;
} Tree;

Tree *tree;

void serialize_node(Node *node, FILE *file) {
  for (int i = 0; i < node->n_children; i++) {
    fprintf(file, "%s	%s\n", node->name, node->children[i].name);
    serialize_node(&node->children[i], file);
  }
}

void serialize_tree(Tree *tree, char *filename) {
  FILE *file = fopen(filename, "w");
  serialize_node(tree->root, file);
  fclose(file);
}

Node *create_node(char *name) {
  Node *node = malloc(sizeof(Node));
  node->name = name;
  node->children = NULL;
  node->n_children = 0;
  return node;
}

Tree *create_tree() {
  Tree *tree = malloc(sizeof(Tree));
  tree->root = create_node("root");
  return tree;
}

void add_child(Node *node, Node *child) {
  node->n_children++;
  node->children = realloc(node->children, node->n_children * sizeof(Node));
  node->children[node->n_children - 1] = *child;
}
