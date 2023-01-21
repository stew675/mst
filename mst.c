// Author: Stew Forster
// Date: 21 Jan 2023
// Free for use for any purpose so long as the authorship is kept intact.

#include	<stdio.h>
#include	<stdint.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<stdbool.h>
#include	<assert.h>

struct mst_node {
	struct mst_node	*l;
	struct mst_node	*r;
	struct mst_node	*p;

	bool		active;

	in_addr_t	width;
	in_addr_t	mask;
	in_addr_t	prefix;
};

struct mst_node root_buf = {0}, *root = &root_buf;
struct mst_node	*hosts = NULL, *tree_nodes = NULL;


uint32_t
width_to_mask(uint32_t width)
{
	uint32_t bm = ~((uint32_t)0);

	assert((width <= 32) && (width >= 16));

	if (width < 32) {
		bm <<= 32 - width;
	}

	return bm;
} // width_to_mask


int
ascii_to_in_addr_t(const char *addr, in_addr_t *ia)
{
	in_addr_t a, b, c, d, in;

	*ia = 0;
	if (sscanf(addr, "%d.%d.%d.%d", &a, &b, &c, &d) != 4) {
		return 0;
	}

	if (a < 0 || a > 255) {
		return 0;
	}
	if (b < 0 || b > 255) {
		return 0;
	}
	if (c < 0 || c > 255) {
		return 0;
	}
	if (d < 0 || d > 255) {
		return 0;
	}

	in = ((a & 0xff) << 24);
	in |= ((b & 0xff) << 16);
	in |= ((c & 0xff) << 8);
	in |= (d & 0xff);

	*ia = in;

	return 1;
} // ascii_to_in_addr_t


struct mst_node *
host_to_node(in_addr_t host)
{
	assert ((host & root->mask) == root->prefix);

	uint32_t index = (uint32_t)(host & ~(root->mask));

	return &(hosts[index]);
} // host_to_index


void
mark_host_up(in_addr_t host)
{
	struct mst_node *node = host_to_node(host);

	node->active = true;

	// Ripple up the status change
	while (node->active && node->p) {
		node = node->p;
		node->active = (node->l->active && node->r->active);
	}
} // mark_host_up


void
mark_host_down(in_addr_t host)
{
	struct mst_node *node = host_to_node(host);

	// Ripple up the status change
	while (node && node->active) {
		node->active = false;
		node = node->p;
	}
} // mark_host_down


void
print_mst(struct mst_node *node)
{
	if (node == NULL) {
		return;
	}

	if (node->active) {
		in_addr_t in = node->prefix;

		printf("%d.%d.%d.%d/%u\n", (in >> 24) & 0xff, (in >> 16) & 0xff,
					   (in >> 8) & 0xff, in & 0xff, node->width);
		return;
	}

	print_mst(node->l);
	print_mst(node->r);
} // print_mst


void
build_host_list(uint32_t num_hosts)
{
	// calloc() implicitly sets active = false, l,r,p to NULL
	hosts = calloc(num_hosts, sizeof(struct mst_node));
	assert(hosts != NULL);

	// Handle num_hosts == 1 special case
	if (num_hosts == 1) {
		memcpy(hosts, root, sizeof(struct mst_node));
		return;
	}

	// All hosts are assumed to be /32.  Initialize them
	for (uint32_t i = 0; i < num_hosts; i++) {
		struct mst_node *n = &hosts[i];

		n->prefix = root->prefix + i;
		n->width = 32;
		n->mask = ~((uint32_t)0);
	}
} // build_host_list()


void
populate_tree(struct mst_node *subs, uint32_t len, struct mst_node *parents)
{
	// Handle when we finally reach the root
	if (len == 1) {
		root->l = &subs[0];
		root->r = &subs[1];
		root->p = NULL;
		root->l->p = root;
		root->r->p = root;
		return;
	}

	// Fill out next level up
	for (uint32_t i = 0; i < len; i++) {
		struct mst_node *p = &parents[i];
		uint32_t si = i << 1;

		p->l = &subs[si];
		p->r = &subs[si + 1];
		p->p = NULL;

		p->active = false;

		p->width = p->l->width - 1;
		p->mask = width_to_mask(p->width);
		p->prefix = p->l->prefix & p->mask;

		// Attach children to parent
		p->l->p = p;
		p->r->p = p;
	}

	// Populate next level up
	populate_tree(parents, len >> 1, &parents[len]);
} // populate_tree


void
build_mst()
{
	uint32_t num_hosts = 1;

	// We only handle up to 2^16 hosts
	if (root->width < 32) {
		num_hosts <<= (32 - root->width);
		assert(num_hosts <= 65536);
		build_host_list(num_hosts);
	} else {
		build_host_list(1);
		return;
	}

	// Allocate up the tree node list
	tree_nodes = calloc(num_hosts, sizeof(struct mst_node));
	assert(tree_nodes != NULL);

	populate_tree(hosts, num_hosts >> 1, tree_nodes);
} // build_mst

void
usage(const char *exec_name)
{
	fprintf(stderr, "%s <network_ipv4_prefix> <network_prefix_width>\n\n", exec_name);
	fprintf(stderr, "The network_prefix_width must be in the range 16..32\n");
	fprintf(stderr, "eg. %s 128.250.1.0 24\n", exec_name);
	exit(1);
} // usage


#define MAX_CMD_LEN	256

int
main(int argc, const char *argv[])
{
	char cmd[MAX_CMD_LEN];

	// First extract the root addr and mask
	if (argc != 3) {
		usage(argv[0]);
	}

	if (ascii_to_in_addr_t(argv[1], &root->prefix) == 0) {
		usage(argv[0]);
	}

	root->width = atoi(argv[2]);
	if ((root->width < 16) || (root->width > 32)) {
		usage(argv[0]);
	}

	root->mask = width_to_mask(root->width);
	root->prefix &= root->mask;

	build_mst();

	// Loop reading input
	// Valid Input
	// "u <address>"	- Marks <address> as up
	// "d <address>"	- Marks <address> as down
	// "p"			- Prints Minimum Routing Spanning Tree
	// "x"			- Exits the program

	while (fgets(cmd, MAX_CMD_LEN, stdin)) {
		char *c;
		in_addr_t host;

		if (cmd[0] == 'x') {
			break;
		}

		switch (cmd[0]) {
		case 'u':
		case 'd':
			c = &cmd[2];
			if (ascii_to_in_addr_t(c, &host) == 0) {
				fprintf(stderr, "Invalid Address Format: %s\n", c);
				break;
			}

			if ((host & root->mask) != root->prefix) {
				fprintf(stderr, "Host Address is not within the sub-network: %s", c);
				break;
			}

			if (cmd[0] == 'u') {
				mark_host_up(host);
			} else {
				mark_host_down(host);
			}

			break;
		case 'p':
			print_mst(root);
			break;
		default:
			fprintf(stderr, "Bad Input: %s", cmd);
			break;
		}
	}
} // main
