/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim_utils.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azahidi <azahidi@student.1337.ma>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/14 19:55:30 by azahidi           #+#    #+#             */
/*   Updated: 2026/11/14 19:55:36 by azahidi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	sim_should_stop(t_sim *sim)
{
	int		stopped;

	pthread_mutex_lock(&sim->sim_mutex);
	stopped = sim->stop_sim;
	pthread_mutex_unlock(&sim->sim_mutex);
	return (stopped);
}

void	sim_set_stop(t_sim *sim)
{
	pthread_mutex_lock(&sim->sim_mutex);
	sim->stop_sim = 1;
	pthread_mutex_unlock(&sim->sim_mutex);
}

long long	get_coder_deadline(t_coder *coder)
{
	long long	deadline;

	pthread_mutex_lock(&coder->state_mutex);
	deadline = coder->last_compile_start + coder->sim->time_to_burnout;
	pthread_mutex_unlock(&coder->state_mutex);
	return (deadline);
}

int	heap_remove_coder(t_heap *heap, int coder_id, t_sched_type sched)
{
	int		i;
	int		last;

	i = 0;
	while (i < heap->size && heap->nodes[i].coder_id != coder_id)
		i++;
	if (i == heap->size)
		return (0);
	last = --heap->size;
	if (i == last)
		return (1);
	heap->nodes[i] = heap->nodes[last];
	if (i > 0 && is_higher_priority(heap->nodes[i], heap->nodes[(i - 1) / 2],
			sched))
		sift_up(heap, i, sched);
	else
		sift_down(heap, i, sched);
	return (1);
}
