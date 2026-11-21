/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   time_utils.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azahidi <azahidi@student.1337.ma>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/14 18:59:24 by azahidi           #+#    #+#             */
/*   Updated: 2026/11/14 19:00:06 by azahidi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long long	get_time_ms(void)
{
	struct timeval	tv;

	if (gettimeofday(&tv, NULL) != 0)
		return (-1);
	return ((long long)tv.tv_sec * 1000LL + (long long)(tv.tv_usec / 1000));
}

long long	get_elapsed_time(long long start_time_ms)
{
	long long	now_ms;

	now_ms = get_time_ms();
	if (now_ms < 0)
		return (-1);
	return (now_ms - start_time_ms);
}

struct timespec	ms_to_timespec(long long epoch_ms)
{
	struct timespec	ts;

	ts.tv_sec = (time_t)(epoch_ms / 1000LL);
	ts.tv_nsec = (long)((epoch_ms % 1000LL) * 1000000LL);
	return (ts);
}

void	smart_sleep(long long duration_ms, t_sim *sim)
{
	long long	target_ms;
	long long	now_ms;

	if (duration_ms <= 0 || sim_should_stop(sim))
		return ;
	now_ms = get_time_ms();
	if (now_ms < 0)
		return ;
	target_ms = now_ms + duration_ms;
	while (1)
	{
		if (sim_should_stop(sim))
			break ;
		now_ms = get_time_ms();
		if (now_ms < 0 || now_ms >= target_ms)
			break ;
		usleep(500);
	}
}
