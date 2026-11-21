/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azahidi <azahidi@student.1337.ma>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 16:52:41 by azahidi           #+#    #+#             */
/*   Updated: 2026/11/16 16:52:43 by azahidi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	is_higher_priority(t_request a, t_request b, t_sched_type sched)
{
	if (sched == SCHED_TYPE_EDF)
	{
		if (a.deadline_ms != b.deadline_ms)
			return (a.deadline_ms < b.deadline_ms);
		if (a.arrival_seq != b.arrival_seq)
			return (a.arrival_seq < b.arrival_seq);
		return (a.coder_id < b.coder_id);
	}
	return (a.arrival_seq < b.arrival_seq);
}

void	sift_up(t_heap *heap, int index, t_sched_type sched)
{
	int			parent;
	t_request	tmp;

	while (index > 0)
	{
		parent = (index - 1) / 2;
		if (!is_higher_priority(heap->nodes[index], heap->nodes[parent], sched))
			break ;
		tmp = heap->nodes[index];
		heap->nodes[index] = heap->nodes[parent];
		heap->nodes[parent] = tmp;
		index = parent;
	}
}

void	sift_down(t_heap *heap, int index, t_sched_type sched)
{
	int			best;
	int			left;
	int			right;
	t_request	tmp;

	while (1)
	{
		best = index;
		left = (2 * index) + 1;
		right = (2 * index) + 2;
		if (left < heap->size && is_higher_priority(heap->nodes[left],
				heap->nodes[best], sched))
			best = left;
		if (right < heap->size && is_higher_priority(heap->nodes[right],
				heap->nodes[best], sched))
			best = right;
		if (best == index)
			break ;
		tmp = heap->nodes[index];
		heap->nodes[index] = heap->nodes[best];
		heap->nodes[best] = tmp;
		index = best;
	}
}

int	heap_push(t_heap *heap, t_request req, t_sched_type sched)
{
	if (heap->size >= heap->capacity)
		return (1);
	heap->nodes[heap->size] = req;
	heap->size++;
	sift_up(heap, heap->size - 1, sched);
	return (0);
}

int	heap_pop(t_heap *heap, t_sched_type sched, t_request *out)
{
	if (heap->size == 0)
		return (1);
	*out = heap->nodes[0];
	heap->size--;
	if (heap->size > 0)
	{
		heap->nodes[0] = heap->nodes[heap->size];
		sift_down(heap, 0, sched);
	}
	return (0);
}
