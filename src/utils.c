#include "retromfa.h"

void	appendNode(t_data *data, t_list *node)
{
	t_list *tmp = data->images;

	if (!tmp)
	{
		data->images = node;
		return ;
	}

	while (tmp->next)
	{
		tmp = tmp->next;
	}
	tmp->next = node;
}

