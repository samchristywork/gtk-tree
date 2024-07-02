#include <cairo/cairo.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <math.h>
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
  int color;
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
double x_offset = 0;
double y_offset = 0;
int current_hash = 0;

void set_color(cairo_t *cr, Color color, double alpha) {
  if (color_scheme == SCHEME_LIGHT) {
    switch (color) {
    case COLOR_ACCENT:
      cairo_set_source_rgba(cr, 1, 0.7, 0.5, alpha);
      break;
    case COLOR_ACCENT_FAINT:
      cairo_set_source_rgba(cr, 1, 0.9, 0.8, alpha);
      break;
    case COLOR_GRID:
      cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, alpha);
      break;
    case COLOR_BACKGROUND:
      cairo_set_source_rgba(cr, 0.95, 0.95, 0.95, alpha);
      break;
    case COLOR_FOREGROUND:
      cairo_set_source_rgba(cr, 0, 0, 0, alpha);
      break;
    }
  } else {
    switch (color) {
    case COLOR_ACCENT:
      cairo_set_source_rgba(cr, 0.4, 0.2, 0.8, alpha);
      break;
    case COLOR_ACCENT_FAINT:
      cairo_set_source_rgba(cr, 0.2, 0.1, 0.4, alpha);
      break;
    case COLOR_GRID:
      cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, alpha);
      break;
    case COLOR_BACKGROUND:
      cairo_set_source_rgba(cr, 0.05, 0.05, 0.05, alpha);
      break;
    case COLOR_FOREGROUND:
      cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, alpha);
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
  node->color = 0;
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
  draw_root = tree->root;
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

void remove_child(Node *node, Node *child) {
  for (int i = 0; i < node->n_children; i++) {
    if (node->children[i] == child) {
      for (int j = i; j < node->n_children - 1; j++) {
        node->children[j] = node->children[j + 1];
      }
      node->n_children--;
      node->children = realloc(node->children, node->n_children * sizeof(Node));
      break;
    }
  }
}

typedef struct string {
  char *str;
  int len;
  int size;
} string;

void serialize_node(Node *node, Node *parent, string *s) {
  if (parent != NULL) {
    if (s->len + 100 > s->size) {
      s->size *= 2;
      s->str = realloc(s->str, s->size);
    }
    s->len +=
        sprintf(s->str + s->len, "edge	%d	%d\n", parent->id, node->id);
    if (s->len + 100 > s->size) {
      s->size *= 2;
      s->str = realloc(s->str, s->size);
    }
    s->len +=
        sprintf(s->str + s->len, "node	%d	%s\n", node->id, node->name);
    if (node->color != 0) {
      if (s->len + 100 > s->size) {
        s->size *= 2;
        s->str = realloc(s->str, s->size);
      }
      s->len +=
          sprintf(s->str + s->len, "color	%d	%d\n", node->id, node->color);
    }
    if (node->filename != NULL) {
      if (s->len + 100 > s->size) {
        s->size *= 2;
        s->str = realloc(s->str, s->size);
      }
      s->len += sprintf(s->str + s->len, "filename	%d	%s\n", node->id,
                        node->filename);
    }
  }
  for (int i = 0; i < node->n_children; i++) {
    serialize_node(&node->children[i], node, s);
  }
}

char *serialize_tree(Node *node) {
  string s;
  s.str = malloc(1000);
  s.size = 1000;
  s.len = 0;

  serialize_node(node, NULL, &s);

  return s.str;
}

int hash_string(char *s) {
  int hash = 0;
  while (*s) {
    hash = hash * 31 + *s;
    s++;
  }
  return hash;
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
    } else if (strcmp(type, "color") == 0) {
      int nodeID;
      int color;
      sscanf(line, "%s\t%d\t%d", type, &nodeID, &color);

      Node *node = find_node(tree->root, nodeID);
      node->color = color;
    } else if (strcmp(type, "node") == 0) {
      int nodeID;
      // TODO: Fix this
      char name[100];
      sscanf(line, "%s\t%d\t%s", type, &nodeID, name);

      Node *node = find_node(tree->root, nodeID);
      node->name = strdup(get_name(line));
    } else if (strcmp(type, "filename") == 0) {
      int nodeID;
      char filename[100];
      sscanf(line, "%s\t%d\t%s", type, &nodeID, filename);

      Node *node = find_node(tree->root, nodeID);
      node->filename = strdup(filename);
    } else {
      printf("Unknown type: %s\n", type);
    }
  }

  fclose(file);

  char *s = serialize_tree(tree->root);
  current_hash = hash_string(s);
  free(s);

  return tree;
}

void draw_rect(cairo_t *cr, Rectangle rect) {
  cairo_set_line_width(cr, 1);
  double width = rect.x2 - rect.x1;
  double height = rect.y2 - rect.y1;
  cairo_rectangle(cr, rect.x1 + x_offset, rect.y1 + y_offset, width, height);
  cairo_stroke(cr);
}

void fill_rect(cairo_t *cr, Rectangle rect) {
  double width = rect.x2 - rect.x1;
  double height = rect.y2 - rect.y1;
  cairo_rectangle(cr, rect.x1 + x_offset, rect.y1 + y_offset, width, height);
  cairo_fill(cr);
}

void draw_circle(cairo_t *cr, double x, double y, double radius) {
  cairo_set_line_width(cr, 1);
  cairo_arc(cr, x + x_offset, y + y_offset, radius, 0, 2 * M_PI);
  cairo_stroke(cr);
}

void fill_circle(cairo_t *cr, double x, double y, double radius) {
  cairo_arc(cr, x + x_offset, y + y_offset, radius, 0, 2 * M_PI);
  cairo_fill(cr);
}

void draw_connector(cairo_t *cr, double x1, double y1, double x2, double y2) {
  x1 += connector_radius;
  x2 -= connector_radius;
  double xm = (x1 + x2) / 2;

  set_color(cr, COLOR_FOREGROUND, 1.0);
  cairo_set_line_width(cr, 1);
  cairo_move_to(cr, x1 + x_offset, y1 + y_offset);
  cairo_curve_to(cr, xm + x_offset, y1 + y_offset, xm + x_offset, y2 + y_offset,
                 x2 + x_offset, y2 + y_offset);
  cairo_stroke(cr);
}

void draw_text(cairo_t *cr, double x, double y, char *text) {
  cairo_move_to(cr, x + x_offset, y + y_offset);
  cairo_show_text(cr, text);
}

void draw_node(cairo_t *cr, Node *node, double x, double y) {
  double xpad = 10;
  double ypad = 10;

  cairo_text_extents_t extents;
  cairo_text_extents(cr, node->name, &extents);
  extents.height = font_size;

  if (node->filename != NULL) {
    set_color(cr, COLOR_ACCENT, 1.0);
    cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 0.15);
    fill_circle(cr, x + extents.width + 2 * xpad, y, 5);
    set_color(cr, COLOR_FOREGROUND, 1.0);
    draw_circle(cr, x + extents.width + 2 * xpad, y, 5);
  }

  if (node->parent != NULL) {
    double x1 = x;
    double y1 = (y + y + extents.height + 2 * ypad) / 2;

    set_color(cr, COLOR_ACCENT, 1.0);
    fill_circle(cr, x1, y1, connector_radius);
    set_color(cr, COLOR_FOREGROUND, 1.0);
    draw_circle(cr, x1, y1, connector_radius);
  }

  if (node->n_children != 0) {
    double x2 = x + extents.width + 2 * xpad;
    double y2 = (y + y + extents.height + 2 * ypad) / 2;

    set_color(cr, COLOR_ACCENT, 1.0);
    fill_circle(cr, x2, y2, connector_radius);
    set_color(cr, COLOR_FOREGROUND, 1.0);
    draw_circle(cr, x2, y2, connector_radius);
  }

  if (node->selected) {
    set_color(cr, COLOR_ACCENT_FAINT, 1.0);
  } else {
    set_color(cr, COLOR_BACKGROUND, 1.0);
  }
  fill_rect(cr, (Rectangle){x, y, x + extents.width + 2 * xpad,
                            y + extents.height + 2 * ypad});

  if (node->color != 0) {
    switch (node->color) {
    case 1:
      cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 0.15);
      break;
    case 2:
      cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.15);
      break;
    case 3:
      cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 0.15);
      break;
    }
    fill_rect(cr, (Rectangle){x, y, x + extents.width + 2 * xpad,
                              y + extents.height + 2 * ypad});
  }

  set_color(cr, COLOR_FOREGROUND, 1.0);
  draw_text(cr, x + xpad, y + ypad + extents.height, node->name);

  set_color(cr, COLOR_FOREGROUND, 1.0);
  draw_rect(cr, (Rectangle){x, y, x + extents.width + 2 * xpad,
                            y + extents.height + 2 * ypad});

  extents.width += 2 * xpad;
  extents.height += 2 * ypad;

  node->rect = (Rectangle){x, y, x + extents.width, y + extents.height};
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
  GtkWidget *dialog =
      gtk_dialog_new_with_buttons("Enter name", NULL, 0, "_OK", 1, NULL);
  GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *entry = gtk_entry_new();
  gtk_container_add(GTK_CONTAINER(content_area), entry);
  gtk_widget_show_all(dialog);

  g_signal_connect(dialog, "key-press-event", G_CALLBACK(handle_return),
                   dialog);

  const char *name;
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

char *compare_strings(char *s1, char *s2) {
  while (*s1 && *s2) {
    if (tolower(*s1) != tolower(*s2)) {
      return NULL;
    }
    s1++;
    s2++;
  }

  if (*s1 || *s2) {
    return NULL;
  }

  return s1;
}

Node *fuzzy_search(Node *node, char *name) {
  if (compare_strings(node->name, name) != NULL) {
    return node;
  }

  for (int i = 0; i < node->n_children; i++) {
    Node *found = fuzzy_search(&node->children[i], name);
    if (found != NULL) {
      return found;
    }
  }

  return NULL;
}

bool is_visible(Rectangle rect, double x_offset, double y_offset, double width,
                double height) {
  int margin = 100;
  if (rect.x1 + x_offset > width - margin || rect.x2 + x_offset < margin ||
      rect.y1 + y_offset > height - margin || rect.y2 + y_offset < margin) {
    return false;
  }

  return true;
}

void center_node(Node *selected) {
  int width;
  int height;
  gtk_window_get_size(GTK_WINDOW(gtk_widget_get_toplevel(drawing_area)), &width,
                      &height);

  y_offset = -selected->rect.y1 + (float)height / 4;
  x_offset = -selected->rect.x1 + (float)width / 8;
}

void show_help() {
  GtkWidget *dialog =
      gtk_dialog_new_with_buttons("Help", NULL, 0, "_OK", 1, NULL);
  GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *label = gtk_label_new("h: Move up\n"
                                   "j: Move down\n"
                                   "k: Move up\n"
                                   "l: Move right\n"
                                   "e: Edit content\n"
                                   "n: Add child\n"
                                   "r: Rename\n"
                                   "d: Delete\n"
                                   "i: Insert\n"
                                   "c: Change color\n"
                                   "z: Center\n"
                                   "s: Save\n"
                                   "S: Print\n"
                                   "q: Quit\n"
                                   "?: Help\n"
                                   "/: Search\n"
                                   "Space: Select random\n"
                                   "Return: Select\n"
                                   "Escape: Quit\n");
  gtk_container_add(GTK_CONTAINER(content_area), label);
  gtk_widget_show_all(dialog);

  int response = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

Node *get_nth_node(Node *node, int *n) {
  if (*n == 0) {
    return node;
  }

  *n = *n - 1;
  for (int i = 0; i < node->n_children; i++) {
    Node *found = get_nth_node(node->children[i], n);
    if (found != NULL) {
      return found;
    }
  }

  return NULL;
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
  case (GDK_KEY_C): {
    if (color_scheme == SCHEME_LIGHT) {
      color_scheme = SCHEME_DARK;
    } else {
      color_scheme = SCHEME_LIGHT;
    }
    break;
  }
  case (GDK_KEY_c): {
    Node *selected = get_selected_node(tree->root);
    if (selected != NULL) {
      selected->color++;
      if (selected->color > 3) {
        selected->color = 0;
      }
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
    } else {
      tree->root->selected = true;
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
    } else {
      tree->root->selected = true;
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
    } else {
      tree->root->selected = true;
    }
    break;
  }
  case (GDK_KEY_e): {
    Node *selected = get_selected_node(tree->root);
    if (selected != NULL) {
      if (selected->filename == NULL) {
        char filename[100];
        sprintf(filename, "content/%s.txt", selected->name);
        selected->filename = strdup(filename);
      }

      if (selected->filename != NULL) {
        FILE *file = fopen(selected->filename, "a");
        fclose(file);

        char command[100];
        sprintf(command, "xdg-open %s", selected->filename);
        printf("%s\n", command);
        system(command);
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
    } else {
      tree->root->selected = true;
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
  case (GDK_KEY_question): {
    show_help();
    break;
  }
  case (GDK_KEY_slash): {
    char *name = ask_for_name();
    Node *node = fuzzy_search(tree->root, name);
    if (node != NULL) {
      Node *selected = get_selected_node(tree->root);
      if (selected != NULL) {
        selected->selected = false;
      }
      node->selected = true;
    }
    break;
  }
  case (GDK_KEY_space): {
    Node *selected = get_selected_node(tree->root);
    if (selected != NULL) {
      selected->selected = false;
    }

    int num_nodes = count_descendents(tree->root);
    int random = rand() % num_nodes;
    Node *node = get_nth_node(tree->root, &random);
    node->selected = true;
    break;
  }
  case (GDK_KEY_z): {
    Node *selected = get_selected_node(tree->root);
    if (selected != NULL) {
      center_node(selected);
    }
    break;
  }
  case (GDK_KEY_s): {
    char *s = serialize_tree(tree->root);
    FILE *file = fopen("tree.txt", "w");
    fprintf(file, "%s", s);
    fclose(file);

    current_hash = hash_string(s);
    free(s);
    break;
  }
  case (GDK_KEY_S): {
    char *s = serialize_tree(tree->root);
    printf("%s", s);
    free(s);
    break;
  }
  }

  gtk_widget_queue_draw(drawing_area);

  Node *selected = get_selected_node(tree->root);
  if (selected != NULL) {
    if (!check_if_descendent(draw_root, selected)) {
      draw_root = selected;
    }

    int width;
    int height;
    gtk_window_get_size(GTK_WINDOW(gtk_widget_get_toplevel(drawing_area)),
                        &width, &height);
    if (!is_visible(selected->rect, x_offset, y_offset, width, height)) {
      center_node(selected);
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

  for (double x = x_offset; x < width; x += xstep) {
    cairo_move_to(cr, x, y_offset);
    cairo_line_to(cr, x, height);
    cairo_stroke(cr);
  }

  for (double y = y_offset; y < height; y += ystep) {
    cairo_move_to(cr, x_offset, y);
    cairo_line_to(cr, width, y);
    cairo_stroke(cr);
  }
}

void draw_background(cairo_t *cr) {
  set_color(cr, COLOR_BACKGROUND, 1.0);
  cairo_paint(cr);

  set_color(cr, COLOR_GRID, 1.0);
  draw_grid(cr, 1, 100, 100);
  draw_grid(cr, 0.5, 20, 20);
}

void draw_frame(cairo_t *cr) {
  static int frame = 0;
  frame++;
  set_color(cr, COLOR_FOREGROUND, 1.0);
  cairo_move_to(cr, 10, 20);
  char text[100];
  sprintf(text, "Frame: %d", frame);
  cairo_show_text(cr, text);
}

void draw_modified_indicator(cairo_t *cr, Tree *tree) {
  char *s = serialize_tree(tree->root);
  if (current_hash != hash_string(s)) {
    set_color(cr, COLOR_FOREGROUND, 1.0);
    char text[100];
    cairo_move_to(cr, 10, 40);
    sprintf(text, "Modified");
    cairo_show_text(cr, text);
  }
  free(s);
}

int count_descendents(Node *node) {
  int count = 0;
  for (int i = 0; i < node->n_children; i++) {
    count += count_descendents(&node->children[i]);
  }
  return count + node->n_children;
}

void draw_side_panel(cairo_t *cr, Tree *tree, double x, double y, double width,
                     double height) {
  set_color(cr, COLOR_BACKGROUND, 0.8);
  cairo_rectangle(cr, x, y, width, height);
  cairo_fill(cr);

  set_color(cr, COLOR_FOREGROUND, 1.0);
  cairo_rectangle(cr, x, y, width, height);
  cairo_stroke(cr);

  Node *selected = get_selected_node(draw_root);
  if (selected != NULL) {
    char text[100];
    int offset = 20;

    sprintf(text, "Node Count: %d", count_descendents(tree->root));
    cairo_move_to(cr, x + 10, y + offset);
    cairo_show_text(cr, text);
    offset += 20;

    sprintf(text, "Name: %s", selected->name);
    cairo_move_to(cr, x + 10, y + offset);
    cairo_show_text(cr, text);
    offset += 20;

    sprintf(text, "Descendents: %d", count_descendents(selected));
    cairo_move_to(cr, x + 10, y + offset);
    cairo_show_text(cr, text);

    if (selected->filename != NULL) {
      offset += 20;
      sprintf(text, "Filename: %s", selected->filename);
      cairo_move_to(cr, x + 10, y + offset);
      cairo_show_text(cr, text);

      FILE *file = fopen(selected->filename, "r");
      if (file != NULL) {
        offset += 20;
        char line[100];
        while (fgets(line, 100, file) != NULL) {
          line[strlen(line) - 1] = '\0';
          cairo_move_to(cr, x + 10, y + offset);
          cairo_show_text(cr, line);
          offset += 20;
        }
        fclose(file);
      }
    }
  }
}

void draw_child_node_names(cairo_t *cr) {
  Node *selected = get_selected_node(draw_root);
  if (selected != NULL) {
    int offset = 60;
    for (int i = 0; i < selected->n_children; i++) {
      cairo_move_to(cr, 10, offset);
      char name[100];
      snprintf(name, 100, "%d: %s", i + 1, selected->children[i]->name);
      cairo_show_text(cr, name);
      offset += 20;
    }
  }
}

static gboolean handle_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
  Tree *tree = (Tree *)data;

  draw_background(cr);

  cairo_set_font_size(cr, font_size);
  draw_nodes(cr, draw_root, 100, 100, 0, 0);

  int panel_width = 600;
  int width;
  int height;
  gtk_window_get_size(GTK_WINDOW(gtk_widget_get_toplevel(drawing_area)), &width,
                      &height);
  draw_side_panel(cr, tree, width - panel_width, 10, panel_width - 10,
                  height - 20);

  draw_child_node_names(cr);

  draw_frame(cr);
  draw_modified_indicator(cr, tree);
  return FALSE;
}

Node *get_clicked_node(Node *node, double x, double y) {
  if (x >= node->rect.x1 && x <= node->rect.x2 && y >= node->rect.y1 &&
      y <= node->rect.y2) {
    return node;
  }

  for (int i = 0; i < node->n_children; i++) {
    Node *clicked = get_clicked_node(&node->children[i], x, y);
    if (clicked != NULL) {
      return clicked;
    }
  }

  return NULL;
}

bool dragging = false;
double mouse_x = 0;
double mouse_y = 0;
double click_pos_x = 0;
double click_pos_y = 0;
static gboolean handle_click(GtkWidget *widget, GdkEventButton *event,
                             gpointer data) {
  Tree *tree = (Tree *)data;

  mouse_x = event->x;
  mouse_y = event->y;
  click_pos_x = event->x;
  click_pos_y = event->y;
  dragging = false;

  return FALSE;
}

static gboolean handle_release(GtkWidget *widget, GdkEventButton *event,
                               gpointer data) {
  Tree *tree = (Tree *)data;

  if (!dragging) {
    Node *selected = get_selected_node(tree->root);
    if (selected != NULL) {
      selected->selected = false;
    }

    Node *clicked =
        get_clicked_node(tree->root, event->x - x_offset, event->y - y_offset);
    if (clicked != NULL) {
      clicked->selected = true;
    }

    gtk_widget_queue_draw(drawing_area);
  }

  return FALSE;
}

double distance(double x1, double y1, double x2, double y2) {
  return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

static gboolean handle_drag(GtkWidget *widget, GdkEventButton *event,
                            gpointer data) {
  Tree *tree = (Tree *)data;

  if (event->state & GDK_BUTTON1_MASK) {
    x_offset += event->x - mouse_x;
    y_offset += event->y - mouse_y;
    mouse_x = event->x;
    mouse_y = event->y;

    if (distance(click_pos_x, click_pos_y, event->x, event->y) > 5) {
      dragging = true;
    }
  }

  gtk_widget_queue_draw(drawing_area);

  return FALSE;
}

int main() {
  Tree *tree = deserialize_tree("tree.txt");

  gtk_init(NULL, NULL);

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Tree Editor");
  gtk_window_set_default_size(GTK_WINDOW(window), 1600, 850);
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  GtkWidget *container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add(GTK_CONTAINER(window), container);

  drawing_area = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(container), drawing_area);
  gtk_widget_set_hexpand(drawing_area, TRUE);
  gtk_widget_set_vexpand(drawing_area, TRUE);

  g_signal_connect(drawing_area, "draw", G_CALLBACK(handle_draw), tree);

  g_signal_connect(window, "key-press-event", G_CALLBACK(handle_key), tree);

  g_signal_connect(window, "button-press-event", G_CALLBACK(handle_click),
                   tree);

  g_signal_connect(window, "button-release-event", G_CALLBACK(handle_release),
                   tree);

  g_signal_connect(window, "motion-notify-event", G_CALLBACK(handle_drag),
                   tree);

  gtk_widget_show_all(window);

  gtk_main();
}
