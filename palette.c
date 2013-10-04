#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "palettes.h"

// No higher than 8, please
#define BIT_DEPTH 8

typedef struct octnode
{
    bool isLeaf;
    size_t numPixels;
    size_t red;
    size_t green;
    size_t blue;
    size_t alpha;
    struct octnode *children[16];
    struct octnode *next;
} octnode;

typedef struct
{
    size_t numLeaves;
    octnode *root;
    octnode *heads[BIT_DEPTH];
} octree;

octnode * octree_create_node(octree *tree, size_t level)
{
    octnode *node = (octnode *) malloc(sizeof(octnode));
    
    if (node == NULL)
    {
        return NULL;
    }
    
    node->red = 0;
    node->green = 0;
    node->blue = 0;
    node->alpha = 0;
    node->numPixels = 0;
    memset(node->children, 0, 16 * sizeof(octnode *));
    
    if (BIT_DEPTH == level)
    {
        node->isLeaf = true;
        tree->numLeaves++;
        node->next = NULL;
    }
    else
    {
        node->isLeaf = false;
        node->next = tree->heads[level];
        tree->heads[level] = node;
    }
    
    return node;
}

void octree_destroy_node(octree *tree, octnode *node, size_t level)
{
    if (node != NULL)
    {
        if (tree->heads[level] == node)
        {
            tree->heads[level] = node->next;
        }
        
        for (size_t i = 0; i < 16; i++)
        {
            if (node->children[i])
            {
                octree_destroy_node(tree, node->children[i], level + 1);
            }
        }
        
        free(node);
    }
}

void octree_add_pixel_to_node(octree *tree, octnode *node, size_t level, const uint8_t *pixel)
{
    static uint8_t mask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
    uint8_t r = pixel[0],
            g = pixel[1],
            b = pixel[2],
            a = pixel[3];
    
    if (node->isLeaf)
    {
        node->numPixels++;
        node->red += r;
        node->green += g;
        node->blue += b;
        node->alpha += a;
    }
    else
    {
        uint8_t shift = 7 - level;
        uint8_t index = ((r & mask[level]) >> shift << 3) |
                        ((g & mask[level]) >> shift << 2) |
                        ((b & mask[level]) >> shift << 1) |
                        ((a & mask[level]) >> shift);
        
        if (!node->children[index])
        {
            node->children[index] = octree_create_node(tree, level + 1);
        }
        
        octree_add_pixel_to_node(tree, node->children[index], level + 1, pixel);
    }
}

octree * octree_create()
{
    octree *tree = (octree *) malloc(sizeof(octree));
    
    if (tree == NULL)
    {
        return NULL;
    }
    
    memset(tree->heads, 0, sizeof(octnode *) * BIT_DEPTH);
    tree->root = octree_create_node(tree, 0);
    
    if (tree->root == NULL)
    {
        free(tree);
        return NULL;
    }
    
    tree->numLeaves = 0;
    
    return tree;
}

void octree_get_leaf_nodes(octree *tree, octnode *node, octnode **leaves, size_t *count)
{
    if (node->isLeaf)
    {
        leaves[*count] = node;
        *count += 1;
    }
    else
    {
        for (size_t i = 0; i < 16; i++)
        {
            if (node->children[i] != NULL)
            {
                octree_get_leaf_nodes(tree, node->children[i], leaves, count);
            }
        }
    }
}

void octree_destroy(octree *tree)
{
    if (tree != NULL)
    {
        octree_destroy_node(tree, tree->root, 0);
        free(tree);
    }
}

void octree_add_pixel(octree *tree, const uint8_t *pixel)
{
    octree_add_pixel_to_node(tree, tree->root, 0, pixel);
}

void octree_reduce(octree *tree)
{
    for (int i = BIT_DEPTH - 1; i > -1; i--)
    {
        if (tree->heads[i])
        {
            octnode *node = tree->heads[i];
            tree->heads[i] = node->next;
            
            for (int j=0; j<16; j++)
            {
                if (node->children[j] != NULL)
                {
                    node->red += node->children[j]->red;
                    node->green += node->children[j]->green;
                    node->blue += node->children[j]->blue;
                    node->alpha += node->children[j]->alpha;
                    node->numPixels += node->children[j]->numPixels;
                    octree_destroy_node(tree, node->children[j], i+1);
                    node->children[j] = NULL;
                    tree->numLeaves--;
                }
            }
            
            node->isLeaf = true;
            tree->numLeaves++;
            
            break;
        }
    }
}

int compare_node_size(const void *a, const void *b)
{
    const octnode **anode = (const octnode **)a;
    const octnode **bnode = (const octnode **)b;
    
    return (*bnode)->numPixels - (*anode)->numPixels;
}

float get_saturation(float r, float g, float b)
{
    float minv = fminf(fminf(r, g), b);
    float maxv = fmaxf(fmaxf(r, g), b);
    
    if (minv == maxv)
    {
        return 0;
    }
    
    float d = maxv - minv;
    float l = (r + r + b + g + g + g) / 6.;
    float s = l > 0.5 ? d / (2 - maxv - minv) : d / (maxv + minv);
    
    return s;
}

void get_rgb(const octnode *node, float *r, float *g, float *b)
{
    *r = (float) ( node->red / node->numPixels ) / 255.;
    *g = (float) ( node->green / node->numPixels ) / 255.;
    *b = (float) ( node->blue / node->numPixels ) / 255.;
}

int compare_node_color(const void *a, const void *b)
{
    const octnode **anode = (const octnode **)a;
    const octnode **bnode = (const octnode **)b;
    
    float ar, ag, ab, br, bg, bb;
    get_rgb(*anode, &ar, &ag, &ab);
    get_rgb(*bnode, &br, &bg, &bb);
    
    float asat = get_saturation(ar, ag, ab);
    float bsat = get_saturation(br, bg, bb);
    
    if (asat > bsat)
    {
        return -1;
    }
    else if (asat < bsat)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void octree_get_colors(octree *tree, uint8_t *colors)
{
    octnode **leaves = calloc(tree->numLeaves, sizeof(octnode *));
    
    if (!leaves)
    {
        return;
    }
    
    size_t count = 0;
    octree_get_leaf_nodes(tree, tree->root, leaves, &count);
    
    for (size_t i = 0; i < count; i++)
    {
        uint8_t *c = colors + i * 4;
        c[0] = leaves[i]->red / leaves[i]->numPixels;
        c[1] = leaves[i]->green / leaves[i]->numPixels;
        c[2] = leaves[i]->blue / leaves[i]->numPixels;
        c[3] = leaves[i]->alpha / leaves[i]->numPixels;
    }
    
    free(leaves);
    
    return;
}

void get_palette(const uint8_t *pixels, size_t numPixels, uint8_t *colors, size_t *numColors)
{
    if (*numColors < 8)
    {
        return;
    }
    
    octree *tree = octree_create();
    
    if (tree == NULL)
    {
        return;
    }
    
    for (size_t i = 0; i < numPixels * 4; i += 4)
    {
        octree_add_pixel(tree, pixels + i);
    }
    
    while (tree->numLeaves > *numColors)
    {
        octree_reduce(tree);
    }
    
    *numColors = tree->numLeaves;
    memset(colors, 0, *numColors * sizeof(uint8_t) * 4);
    octree_get_colors(tree, colors);
}

void get_dominant_color(const uint8_t *pixels, size_t numPixels, uint8_t *color)
{
    octree *tree = octree_create();
    
    if (tree == NULL)
    {
        return;
    }
    
    for (size_t i = 0; i < numPixels * 4; i += 4)
    {
        octree_add_pixel(tree, pixels + i);
    }
    
    while (tree->numLeaves > 16)
    {
        octree_reduce(tree);
    }
    
    octnode **leaves = calloc(tree->numLeaves, sizeof(octnode *));
    
    if (leaves == NULL)
    {
        return;
    }
    
    size_t count = 0;
    octree_get_leaf_nodes(tree, tree->root, leaves, &count);
    
    qsort(leaves, tree->numLeaves, sizeof(octnode *), compare_node_size);
    qsort(leaves, tree->numLeaves/4, sizeof(octnode *), compare_node_color);
    
    octnode *leaf = leaves[0];
    
    color[0] = leaf->red / leaf->numPixels;
    color[1] = leaf->green / leaf->numPixels;
    color[2] = leaf->blue / leaf->numPixels;
    color[3] = leaf->alpha / leaf->numPixels;
}
