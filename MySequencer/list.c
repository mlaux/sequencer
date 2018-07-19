
struct list_node {
	void *data;
	struct list_node *next;
};

void list_add(struct list_node *list, void *data)
{
	struct list_node *new_node = malloc(sizeof *new_node);
	new_node->data = data;
	new_node->next = (struct list_node *) 0;
	list->next = new_node;
	return new_node;
}

void list_remove(struct list_node *node)
{
	struct list_node *old_next = node->next;
}

void list_foreach(struct list_node *list, int (*fn)(struct list_node *))
{
	struct list_node *p = list;
	while (p) {
		if (fn(p)) {
			break;
		}
		p = p->next;
	}
}