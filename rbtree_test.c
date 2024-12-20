#include <stdio.h>
#include <stdlib.h>

#define K_RBTREE_RED      0
#define K_RBTREE_BLACK    1

struct k_rbtree_node_t {
    unsigned long rbt_parent_color;
    struct k_rbtree_node_t *rbt_left;
    struct k_rbtree_node_t *rbt_right;
    int key;  // Added for testing purposes
};

struct k_rbtree_root_t {
    struct k_rbtree_node_t *rbt_node;
};

#define K_RBTREE_PARENT(r)    ((struct k_rbtree_node_t *)((r)->rbt_parent_color & ~3))
#define K_RBTREE_COLOR(r)     ((r)->rbt_parent_color & 1)
#define K_RBTREE_IS_RED(r)    (!K_RBTREE_COLOR(r))
#define K_RBTREE_IS_BLACK(r)  K_RBTREE_COLOR(r)

static inline void rbtree_set_parent(struct k_rbtree_node_t *rb, struct k_rbtree_node_t *p)
{
    rb->rbt_parent_color = (rb->rbt_parent_color & 3) | (unsigned long)p;
}

static inline void rbtree_set_color(struct k_rbtree_node_t *rb, int color)
{
    rb->rbt_parent_color = (rb->rbt_parent_color & ~1) | color;
}

static inline void rbtree_set_parent_color(struct k_rbtree_node_t *rb, struct k_rbtree_node_t *p, int color)
{
    rb->rbt_parent_color = (unsigned long)p | color;
}

static inline void rbtree_set_black(struct k_rbtree_node_t *rb)
{
    rb->rbt_parent_color |= K_RBTREE_BLACK;
}

static inline struct k_rbtree_node_t *rbtree_parent(const struct k_rbtree_node_t *node)
{
    return (struct k_rbtree_node_t *)(node->rbt_parent_color & ~3);
}

static void rbtree_change_child(struct k_rbtree_node_t *old, struct k_rbtree_node_t *new,
                              struct k_rbtree_node_t *parent, struct k_rbtree_root_t *root)
{
    if (parent) {
        if (parent->rbt_left == old)
            parent->rbt_left = new;
        else
            parent->rbt_right = new;
    } else
        root->rbt_node = new;
}

static void rbtree_rotate_set_parents(struct k_rbtree_node_t *old, struct k_rbtree_node_t *new,
                                    struct k_rbtree_root_t *root, int color)
{
    struct k_rbtree_node_t *parent = rbtree_parent(old);
    new->rbt_parent_color = old->rbt_parent_color;
    rbtree_set_parent_color(old, new, color);
    rbtree_change_child(old, new, parent, root);
}

void rbtree_insert_color(struct k_rbtree_node_t *node, struct k_rbtree_root_t *root)
{
    struct k_rbtree_node_t *parent = rbtree_parent(node), *gparent, *tmp;

    while (1) {
        if (!parent) {
            rbtree_set_parent_color(node, NULL, K_RBTREE_BLACK);
            break;
        }
        if (K_RBTREE_IS_BLACK(parent))
            break;

        gparent = rbtree_parent(parent);
        tmp = gparent->rbt_right;
        
        if (parent != tmp) {
            if (tmp && K_RBTREE_IS_RED(tmp)) {
                rbtree_set_parent_color(tmp, gparent, K_RBTREE_BLACK);
                rbtree_set_parent_color(parent, gparent, K_RBTREE_BLACK);
                node = gparent;
                parent = rbtree_parent(node);
                rbtree_set_parent_color(node, parent, K_RBTREE_RED);
                continue;
            }

            tmp = parent->rbt_right;
            if (node == tmp) {
                tmp = node->rbt_left;
                parent->rbt_right = tmp;
                node->rbt_left = parent;
                if (tmp)
                    rbtree_set_parent_color(tmp, parent, K_RBTREE_BLACK);
                rbtree_set_parent_color(parent, node, K_RBTREE_RED);
                parent = node;
                tmp = node->rbt_right;
            }

            gparent->rbt_left = tmp;
            parent->rbt_right = gparent;
            if (tmp)
                rbtree_set_parent_color(tmp, gparent, K_RBTREE_BLACK);
            rbtree_rotate_set_parents(gparent, parent, root, K_RBTREE_RED);
            break;
        } else {
            tmp = gparent->rbt_left;
            if (tmp && K_RBTREE_IS_RED(tmp)) {
                rbtree_set_parent_color(tmp, gparent, K_RBTREE_BLACK);
                rbtree_set_parent_color(parent, gparent, K_RBTREE_BLACK);
                node = gparent;
                parent = rbtree_parent(node);
                rbtree_set_parent_color(node, parent, K_RBTREE_RED);
                continue;
            }

            tmp = parent->rbt_left;
            if (node == tmp) {
                tmp = node->rbt_right;
                parent->rbt_left = tmp;
                node->rbt_right = parent;
                if (tmp)
                    rbtree_set_parent_color(tmp, parent, K_RBTREE_BLACK);
                rbtree_set_parent_color(parent, node, K_RBTREE_RED);
                parent = node;
                tmp = node->rbt_left;
            }

            gparent->rbt_right = tmp;
            parent->rbt_left = gparent;
            if (tmp)
                rbtree_set_parent_color(tmp, gparent, K_RBTREE_BLACK);
            rbtree_rotate_set_parents(gparent, parent, root, K_RBTREE_RED);
            break;
        }
    }
}

// Function to create a new node
struct k_rbtree_node_t *create_node(int key)
{
    struct k_rbtree_node_t *node = malloc(sizeof(struct k_rbtree_node_t));
    node->key = key;
    node->rbt_left = NULL;
    node->rbt_right = NULL;
    node->rbt_parent_color = K_RBTREE_RED;  // New nodes are red
    return node;
}

// Function to insert a new key
void insert_key(struct k_rbtree_root_t *root, int key)
{
    struct k_rbtree_node_t *node = create_node(key);
    struct k_rbtree_node_t *parent = NULL;
    struct k_rbtree_node_t **p = &root->rbt_node;

    while (*p) {
        parent = *p;
        if (key < parent->key)
            p = &parent->rbt_left;
        else
            p = &parent->rbt_right;
    }

    node->rbt_parent_color = (unsigned long)parent;
    *p = node;

    rbtree_insert_color(node, root);
}

// Function to print the tree in-order
void print_inorder(struct k_rbtree_node_t *node)
{
    if (node) {
        print_inorder(node->rbt_left);
        printf("%d(%s) ", node->key, K_RBTREE_IS_RED(node) ? "R" : "B");
        print_inorder(node->rbt_right);
    }
}

int main()
{
    struct k_rbtree_root_t root = {NULL};

    printf("Inserting numbers into Red-Black Tree...\n");
    
    // Test inserting some numbers
    insert_key(&root, 10);
    insert_key(&root, 5);
    insert_key(&root, 15);
    insert_key(&root, 3);
    insert_key(&root, 7);
    insert_key(&root, 12);
    insert_key(&root, 18);

    printf("\nInorder traversal of the tree (with colors):\n");
    printf("Format: number(color) where R=Red, B=Black\n");
    print_inorder(root.rbt_node);
    printf("\n");

    return 0;
}
