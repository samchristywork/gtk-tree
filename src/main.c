#include <cairo/cairo.h>
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define M_PI 3.14159265358979323846

typedef struct Rectangle {
  double x1;
  double y1;
  double x2;
  double y2;
} Rectangle;

typedef struct Node {
  char *name;
  struct Node *children;
  int n_children;
  Rectangle rect;
  bool selected;
  int id;
  struct Node *parent;
  char *filename;
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

typedef enum Scheme {
  SCHEME_LIGHT,
  SCHEME_DARK,
} Scheme;

GtkWidget *drawing_area;
double font_size = 12;
Node *draw_root;
double connector_radius = 4;
Scheme color_scheme = SCHEME_DARK;

void set_color(cairo_t *cr, Color color) {
  if (color_scheme == SCHEME_LIGHT) {
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
  } else {
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
  }
}

Node *create_node(int id) {
  Node *node = malloc(sizeof(Node));
  node->name = NULL;
  node->children = NULL;
  node->n_children = 0;
  node->rect = (Rectangle){0, 0, 0, 0};
  node->selected = false;
  node->parent = NULL;
  node->id = id;
  node->filename = NULL;
  return node;
}

Node *find_node(Node *node, int id) {
  if (node->id == id) {
    return node;
  }

  for (int i = 0; i < node->n_children; i++) {
    Node *found = find_node(&node->children[i], id);
    if (found != NULL) {
      return found;
    }
  }

  return NULL;
}

Tree *create_tree() {
  Tree *tree = malloc(sizeof(Tree));
  tree->root = create_node(0);
  tree->root->name = strdup("root");
  return tree;
}

void add_child(Node *node, Node *child) {
  node->n_children++;
  node->children = realloc(node->children, node->n_children * sizeof(Node));
  node->children[node->n_children - 1] = *child;
}

char *get_name(char *line) {
  char *name = strchr(line, '\t');
  name = strchr(name + 1, '\t');
  return name + 1;
}

void serialize_node(Node *node, Node *parent, FILE *file) {
  if (parent != NULL) {
    fprintf(file, "edge	%d	%d\n", parent->id, node->id);
    fprintf(file, "node	%d	%s\n", node->id, node->name);
  }
  for (int i = 0; i < node->n_children; i++) {
    serialize_node(&node->children[i], node, file);
  }
}

void serialize_tree(Tree *tree, char *filename) {
  FILE *file = fopen(filename, "w");
  serialize_node(tree->root, NULL, file);
  fclose(file);
}

Tree *deserialize_tree(char *filename) {
  FILE *file = fopen(filename, "r");

  Tree *tree = create_tree();
  tree->root->selected = true;

  // TODO: Fix this
  char line[100];
  // TODO: Fix this
  int line_number = 0;
  while (fgets(line, 100, file) != NULL) {
    line_number++;
    if (strlen(line) == 99) {
      printf("Line %d is too long\n", line_number);
      exit(EXIT_FAILURE);
    }

    char type[100];
    sscanf(line, "%s", type);

    line[strlen(line) - 1] = '\0';

    if (strcmp(type, "edge") == 0) {
      int parentID;
      int childID;
      sscanf(line, "%s	%d	%d", type, &parentID, &childID);

      Node *parentNode = find_node(tree->root, parentID);
      Node *childNode = create_node(childID);

      add_child(parentNode, childNode);
    } else if (strcmp(type, "node") == 0) {
      int nodeID;
      // TODO: Fix this
      char name[100];
      sscanf(line, "%s\t%d\t%s", type, &nodeID, name);

      Node *node = find_node(tree->root, nodeID);
      node->name = strdup(get_name(line));
    } else {
      printf("Unknown type: %s\n", type);
    }
  }

  fclose(file);

  return tree;
}

void draw_rect(cairo_t *cr, Rectangle rect) {
  cairo_set_line_width(cr, 1);
  double width = rect.x2 - rect.x1;
  double height = rect.y2 - rect.y1;
  cairo_rectangle(cr, rect.x1, rect.y1, width, height);
  cairo_stroke(cr);
}

void fill_rect(cairo_t *cr, Rectangle rect) {
  double width = rect.x2 - rect.x1;
  double height = rect.y2 - rect.y1;
  cairo_rectangle(cr, rect.x1, rect.y1, width, height);
  cairo_fill(cr);
}

void draw_node(cairo_t *cr, Node *node, double x, double y) {
  double xpad = 10;
  double ypad = 10;

  cairo_text_extents_t extents;
  cairo_text_extents(cr, node->name, &extents);
  extents.height = font_size;

  if (node->parent != NULL) {
    double x1 = x;
    double y1 = (y + y + extents.height + 2 * ypad) / 2;
    set_color(cr, COLOR_ACCENT);
    cairo_set_line_width(cr, 1);
    cairo_arc(cr, x1, y1, connector_radius, 0, 2 * M_PI);
    cairo_fill(cr);

    set_color(cr, COLOR_FOREGROUND);
    cairo_set_line_width(cr, 1);
    cairo_arc(cr, x1, y1, connector_radius, 0, 2 * M_PI);
    cairo_stroke(cr);
  }

  if (node->n_children != 0) {
    double x2 = x + extents.width + 2 * xpad;
    double y2 = (y + y + extents.height + 2 * ypad) / 2;
    set_color(cr, COLOR_ACCENT);
    cairo_set_line_width(cr, 1);
    cairo_arc(cr, x2, y2, connector_radius, 0, 2 * M_PI);
    cairo_fill(cr);

    set_color(cr, COLOR_FOREGROUND);
    cairo_set_line_width(cr, 1);
    cairo_arc(cr, x2, y2, connector_radius, 0, 2 * M_PI);
    cairo_stroke(cr);
  }

  if (node->selected) {
    set_color(cr, COLOR_ACCENT_FAINT);
  } else {
    set_color(cr, COLOR_BACKGROUND);
  }
  fill_rect(cr, (Rectangle){x, y, x + extents.width + 2 * xpad,
                            y + extents.height + 2 * ypad});

  set_color(cr, COLOR_FOREGROUND);
  cairo_move_to(cr, x + xpad, y + ypad + extents.height);
  cairo_show_text(cr, node->name);

  set_color(cr, COLOR_FOREGROUND);
  draw_rect(cr, (Rectangle){x, y, x + extents.width + 2 * xpad,
                            y + extents.height + 2 * ypad});
  extents.width += 2 * xpad;
  extents.height += 2 * ypad;

  node->rect = (Rectangle){x, y, x + extents.width, y + extents.height};
}

Node *get_selected_node(Node *node) {
  if (node->selected) {
    return node;
  }

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
