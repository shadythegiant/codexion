/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azahidi <azahidi@student.1337.ma>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 16:52:22 by azahidi           #+#    #+#             */
/*   Updated: 2026/11/16 16:52:25 by azahidi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	join_threads(t_sim *sim)
{
	int	i;

	pthread_join(sim->monitor_thread, NULL);
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
}

int	main(int argc, char **argv)
{
	t_sim	sim;

	if (parse_args(argc, argv, &sim) != 0)
		return (fprintf(stderr, "Error: invalid arguments\n"), 1);
	if (init_sim(&sim) != 0)
		return (fprintf(stderr, "Error: initialization failed\n"), 1);
	if (start_thread(&sim) != 0)
		return (cleanup_sim(&sim), 1);
	join_threads(&sim);
	cleanup_sim(&sim);
	return (0);
}
