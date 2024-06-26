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

void draw_connector(cairo_t *cr, double x1, double y1, double x2, double y2) {
  x1 += connector_radius;
  x2 -= connector_radius;
  double xm = (x1 + x2) / 2;

  set_color(cr, COLOR_FOREGROUND);
  cairo_set_line_width(cr, 1);
  cairo_move_to(cr, x1, y1);
  cairo_curve_to(cr, xm, y1, xm, y2, x2, y2);
  cairo_stroke(cr);
}

Rectangle draw_nodes(cairo_t *cr, Node *node, double x, double y,
                     double parent_x, double parent_y) {

  double xmargin = 50;
  double ymargin = 10;

  draw_node(cr, node, x, y);

  if (parent_x != 0 && parent_y != 0) {
    draw_connector(cr, parent_x, parent_y, node->rect.x1,
                   (node->rect.y1 + node->rect.y2) / 2);
  }

  Rectangle rect = node->rect;

  for (int i = 0; i < node->n_children; i++) {
    node->children[i].parent = node;
    Rectangle child_rect =
        draw_nodes(cr, &node->children[i], node->rect.x2 + xmargin, y,
                   node->rect.x2, (node->rect.y1 + node->rect.y2) / 2);
    y = child_rect.y2 + ymargin;

    if (child_rect.y2 > rect.y2) {
      rect.y2 = child_rect.y2;
    }

    if (child_rect.x2 > rect.x2) {
      rect.x2 = child_rect.x2;
    }
  }

  return rect;
}

Node *get_selected_node(Node *node) {
  if (node->selected) {
    return node;
  }

  for (int i = 0; i < node->n_children; i++) {
    Node *selected = get_selected_node(&node->children[i]);
    if (selected != NULL) {
      return selected;
    }
  }

  return NULL;
}

static gboolean handle_return(GtkWidget *widget, GdkEventKey *event,
                              gpointer data) {
  if (event->keyval == GDK_KEY_Return) {
    gtk_dialog_response(GTK_DIALOG(data), 1);
    return TRUE;
  }

  return FALSE;
}

char *ask_for_name() {
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *entry;
  const char *name;

  dialog = gtk_dialog_new_with_buttons("Enter name", NULL, 0, "_OK", 1, NULL);
  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  entry = gtk_entry_new();
  gtk_container_add(GTK_CONTAINER(content_area), entry);
  gtk_widget_show_all(dialog);

  g_signal_connect(dialog, "key-press-event", G_CALLBACK(handle_return),
                   dialog);

  int response = gtk_dialog_run(GTK_DIALOG(dialog));
  if (response == 1) {
    name = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
  } else {
    name = NULL;
  }

  gtk_widget_destroy(dialog);

  return (char *)name;
}

bool ask_yes_no(char *question) {
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *label;

  dialog = gtk_dialog_new_with_buttons("Question", NULL, 0, "_Yes", 1, "_No", 0,
                                       NULL);
  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  label = gtk_label_new(question);
  gtk_container_add(GTK_CONTAINER(content_area), label);
  gtk_widget_show_all(dialog);

  int response = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  return response == 1;
}

int get_unused_id(Node *node) {
  int id = 0;
  while (find_node(node, id) != NULL) {
    id++;
  }
  return id;
}

bool check_if_descendent(Node *root, Node *node) {
  if (root == node) {
    return true;
  }

  for (int i = 0; i < root->n_children; i++) {
    if (check_if_descendent(&root->children[i], node)) {
      return true;
    }
  }

  return false;
}

static gboolean handle_key(GtkWidget *widget, GdkEventKey *event,
                           gpointer data) {
  Tree *tree = (Tree *)data;

  switch (event->keyval) {
  case (GDK_KEY_Escape): {
    gtk_main_quit();
    return TRUE;
    break;
  }
  case (GDK_KEY_q): {
    gtk_main_quit();
    return TRUE;
    break;
  }
  case (GDK_KEY_c): {
    if (color_scheme == SCHEME_LIGHT) {
      color_scheme = SCHEME_DARK;
    } else {
      color_scheme = SCHEME_LIGHT;
    }
    break;
  }
  case (GDK_KEY_h): {
    Node *selected = get_selected_node(tree->root);
    if (selected != NULL) {
      if (selected->parent != NULL) {
        selected->selected = false;
        selected->parent->selected = true;
      }
    }
    break;
  }
  case (GDK_KEY_l): {
    Node *selected = get_selected_node(tree->root);
    if (selected != NULL) {
      if (selected->n_children > 0) {
        selected->selected = false;
        selected->children[0].selected = true;
      }
    }
    break;
  }
  case (GDK_KEY_j): {
    Node *selected = get_selected_node(tree->root);
    if (selected != NULL) {
      if (selected->parent != NULL) {
        for (int i = 0; i < selected->parent->n_children; i++) {
          if (&selected->parent->children[i] == selected) {
            if (i < selected->parent->n_children - 1) {
              selected->selected = false;
              selected->parent->children[i + 1].selected = true;
            }
          }
        }
      }
    }
    break;
  }
  case (GDK_KEY_k): {
    Node *selected = get_selected_node(tree->root);
    if (selected != NULL) {
      if (selected->parent != NULL) {
        for (int i = 0; i < selected->parent->n_children; i++) {
          if (&selected->parent->children[i] == selected) {
            if (i > 0) {
              selected->selected = false;
              selected->parent->children[i - 1].selected = true;
            }
          }
        }
      }
    }
    break;
  }
  case (GDK_KEY_n): {
    Node *selected = get_selected_node(tree->root);
    if (selected != NULL) {
      char *name = ask_for_name();
      if (name) {
        Node *new_node = create_node(get_unused_id(tree->root));
        new_node->name = strdup(name);
        add_child(selected, new_node);
      }
    }
    break;
  }
  case (GDK_KEY_r): {
    Node *selected = get_selected_node(tree->root);
    if (selected != NULL) {
      char *name = ask_for_name();
      if (name) {
        selected->name = name;
      }
    }
    break;
  }
  case (GDK_KEY_d): {
    Node *selected = get_selected_node(tree->root);
    if (selected != NULL) {
      Node *parent = selected->parent;
      if (parent != NULL) {
        if (selected->n_children > 0) {
          if (!ask_yes_no("Delete node with children?")) {
            break;
          }
        }
        for (int i = 0; i < parent->n_children; i++) {
          if (&parent->children[i] == selected) {
            for (int j = i; j < parent->n_children - 1; j++) {
              parent->children[j] = parent->children[j + 1];
            }
            parent->n_children--;
            parent->children =
                realloc(parent->children, parent->n_children * sizeof(Node));
            break;
          }
        }
        parent->selected = true;
      }
    }
    break;
  }
  case (GDK_KEY_i): {
    Node *selected = get_selected_node(tree->root);
    if (selected != NULL) {
      char *name = ask_for_name();
      if (name) {
        Node *new_node = create_node(get_unused_id(tree->root));
        new_node->name = strdup(name);
        add_child(new_node, selected);

        Node *parent = selected->parent;
        if (parent != NULL) {
          for (int i = 0; i < parent->n_children; i++) {
            if (&parent->children[i] == selected) {
              parent->children[i] = *new_node;
              break;
            }
          }
        }
      }
    }
    break;
  }
  case (GDK_KEY_Return): {
    Node *selected = get_selected_node(tree->root);
    if (selected != NULL) {
      draw_root = selected;
    }
    break;
  }
  case (GDK_KEY_s): {
    serialize_tree(tree, "tree.txt");
    tree = deserialize_tree("tree.txt");
    break;
  }
  case (GDK_KEY_S): {
    serialize_tree(tree, "/dev/stdout");
    break;
  }
  }

  gtk_widget_queue_draw(drawing_area);

  return FALSE;
}

void draw_grid(cairo_t *cr, double line_width, double xstep, double ystep) {
  cairo_set_line_width(cr, line_width);

  int width;
  int height;
  gtk_window_get_size(GTK_WINDOW(gtk_widget_get_toplevel(drawing_area)), &width,
                      &height);

  for (double x = 0; x < width; x += xstep) {
    cairo_move_to(cr, x, 0);
    cairo_line_to(cr, x, height);
    cairo_stroke(cr);
  }

  for (double y = 0; y < height; y += ystep) {
    cairo_move_to(cr, 0, y);
    cairo_line_to(cr, width, y);
    cairo_stroke(cr);
  }
}

void draw_background(cairo_t *cr) {
  set_color(cr, COLOR_BACKGROUND);
  cairo_paint(cr);

  set_color(cr, COLOR_GRID);
  draw_grid(cr, 1, 100, 100);
  draw_grid(cr, 0.5, 20, 20);
}

void draw_frame(cairo_t *cr) {
  static int frame = 0;
  frame++;
  set_color(cr, COLOR_FOREGROUND);
  cairo_move_to(cr, 10, 20);
  char text[100];
  sprintf(text, "Frame: %d", frame);
  cairo_show_text(cr, text);
}

static gboolean handle_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
  draw_background(cr);

  cairo_set_font_size(cr, font_size);
  draw_nodes(cr, draw_root, 100, 100, 0, 0);

  draw_frame(cr);
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
