#include <cairo/cairo.h>
#include <gtk/gtk.h>
#include <stdbool.h>
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

typedef enum Color {
  COLOR_ACCENT,
  COLOR_ACCENT_FAINT,
  COLOR_GRID,
  COLOR_BACKGROUND,
  COLOR_FOREGROUND,
} Color;

void serialize_node(Node *node, FILE *file) {
  for (int i = 0; i < node->n_children; i++) {
    fprintf(file, "%s	%s\n", node->name, node->children[i].name);
    serialize_node(&node->children[i], file);
  }
}

void set_color(cairo_t *cr, Color color) {
#ifdef COLOR_SCHEME_LIGHT
  switch (color) {
  case COLOR_ACCENT:
    cairo_set_source_rgb(cr, 1, 0.7, 0.5);
    break;
  case COLOR_ACCENT_FAINT:
    cairo_set_source_rgb(cr, 1, 0.9, 0.8);
    break;
  case COLOR_GRID:
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    break;
  case COLOR_BACKGROUND:
    cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
    break;
  case COLOR_FOREGROUND:
    cairo_set_source_rgb(cr, 0, 0, 0);
    break;
  }
#else
  switch (color) {
  case COLOR_ACCENT:
    cairo_set_source_rgb(cr, 0.4, 0.2, 0.8);
    break;
  case COLOR_ACCENT_FAINT:
    cairo_set_source_rgb(cr, 0.2, 0.1, 0.4);
    break;
  case COLOR_GRID:
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    break;
  case COLOR_BACKGROUND:
    cairo_set_source_rgb(cr, 0.05, 0.05, 0.05);
    break;
  case COLOR_FOREGROUND:
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    break;
  }
#endif
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

int main() {
  tree = create_tree();
  add_child(tree->root, create_node("child1"));
  add_child(tree->root, create_node("child2"));
  add_child(&tree->root->children[0], create_node("child3"));
  add_child(&tree->root->children[0], create_node("child4"));
  add_child(&tree->root->children[1], create_node("child5"));

  serialize_tree(tree, "tree.txt");

  gtk_init(NULL, NULL);

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Hello, World!");
  gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  GtkWidget *container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window), container);

  GtkWidget *button = gtk_button_new_with_label("Hello, World!");
  gtk_container_add(GTK_CONTAINER(container), button);
  g_signal_connect(button, "clicked", G_CALLBACK(gtk_main_quit), NULL);

  GtkWidget *drawing_area = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(container), drawing_area);
  gtk_widget_set_size_request(drawing_area, 800, 400);
  g_signal_connect(drawing_area, "draw", G_CALLBACK(handle_draw), NULL);

  g_signal_connect(window, "key-press-event", G_CALLBACK(handle_key), NULL);

  gtk_widget_show_all(window);
  gtk_main();
}
