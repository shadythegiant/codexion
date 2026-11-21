/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azahidi <azahidi@student.1337.ma>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/18 20:34:15 by azahidi           #+#    #+#             */
/*   Updated: 2026/11/18 20:34:19 by azahidi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	parse_ll(const char *str, long long *out, long long min_val)
{
	long long		val;
	int				i;

	if (!str || str[0] == '\0')
		return (0);
	i = 0;
	if (str[i] == '\0')
		return (0);
	val = 0;
	while (str[i])
	{
		if (str[i] < '0' || str[i] > '9')
			return (0);
		if (val > (LLONG_MAX - (str[i] - '0')) / 10)
			return (0);
		val = (val * 10) + (str[i] - '0');
		i++;
	}
	if (val < min_val)
		return (0);
	*out = val;
	return (1);
}

static int	parse_scheduler(const char *str, t_sched_type *sched)
{
	if (!str)
		return (0);
	if (strcmp(str, "fifo") == 0)
	{
		*sched = SCHED_TYPE_FIFO;
		return (1);
	}
	if (strcmp(str, "edf") == 0)
	{
		*sched = SCHED_TYPE_EDF;
		return (1);
	}
	return (0);
}

int	parse_args(int argc, char **argv, t_sim *sim)
{
	long long	tmp_coders;
	long long	tmp_compiles;

	if (argc != 9)
		return (1);
	memset(sim, 0, sizeof(t_sim));
	if (!parse_ll(argv[1], &tmp_coders, 1) || tmp_coders > INT_MAX
		|| !parse_ll(argv[2], &sim->time_to_burnout, 1) || !parse_ll(argv[3],
			&sim->time_to_compile, 0) || !parse_ll(argv[4], &sim->time_to_debug,
			0) || !parse_ll(argv[5], &sim->time_to_refactor, 0)
		|| !parse_ll(argv[6], &tmp_compiles, 1) || tmp_compiles > INT_MAX
		|| !parse_ll(argv[7], &sim->dongle_cooldown, 0)
		|| !parse_scheduler(argv[8], &sim->scheduler))
		return (1);
	sim->num_coders = (int)tmp_coders;
	sim->max_compiles = (int)tmp_compiles;
	return (0);
}
