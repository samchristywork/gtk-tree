#include <cairo/cairo.h>
#include <gtk/gtk.h>
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

int draw_node(cairo_t *cr, Node *node, int x, int y) {
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, node->name);

  cairo_text_extents_t extents;
  cairo_text_extents(cr, node->name, &extents);
  x += extents.width + 10;

  int y_offset = 10;
  for (int i = 0; i < node->n_children; i++) {
    y_offset += draw_node(cr, &node->children[i], x, y + y_offset - 10);
  }

  return y_offset;
}

void draw_tree(cairo_t *cr) { draw_node(cr, tree->root, 100, 100); }

static gboolean handle_key(GtkWidget *widget, GdkEventKey *event,
                           gpointer data) {
  if (event->keyval == GDK_KEY_Escape) {
    gtk_main_quit();
    return TRUE;
  } else if (event->keyval == GDK_KEY_q) {
    gtk_main_quit();
    return TRUE;
  }
  return FALSE;
}

void draw_background(cairo_t *cr) {
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_paint(cr);
}

static gboolean handle_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
  draw_background(cr);
  draw_tree(cr);
  return FALSE;
}
