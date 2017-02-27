
/* 
 * Copyright (C) 2010, 2011  Cesar Rodriguez <cesar.rodriguez@lsv.ens-cachan.fr>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/* see src/verbosity.h for more info */
#define CONFIG_MAX_VERB_LEVEL 1

/* test and debug */
#undef CONFIG_DEBUG

#define CONFIG_MAX_PROCESSES 50
#define CONFIG_MAX_EVENTS_PER_PROCCESS 40000

#define CONFIG_GUEST_DEFAULT_MEMORY_SIZE (64 << 20)
#define CONFIG_GUEST_DEFAULT_THREAD_STACK_SIZE (4 << 20)
#define CONFIG_GUEST_TRACE_BUFFER_SIZE (8 << 20)

