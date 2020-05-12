/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>

#include "../util.h"

#if defined(__linux__)
	#include <stdint.h>

	const char *
	ram_free(void)
	{
		uintmax_t freem;

		if (pscanf("/proc/meminfo",
		           "MemTotal: %ju kB\n"
		           "MemFree: %ju kB\n"
		           "MemAvailable: %ju kB\n",
		           &freem, &freem, &freem) != 3) {
			return NULL;
		}

		return fmt_human(freem * 1024, 1024);
	}

	const char *
	ram_perc(void)
	{
		uintmax_t total, freem, buffers, cached;

		if (pscanf("/proc/meminfo",
		           "MemTotal: %ju kB\n"
		           "MemFree: %ju kB\n"
		           "MemAvailable: %ju kB\n"
		           "Buffers: %ju kB\n"
		           "Cached: %ju kB\n",
		           &total, &freem, &buffers, &buffers, &cached) != 5) {
			return NULL;
		}

		if (total == 0) {
			return NULL;
		}

		return bprintf("%d", 100 * ((total - freem) - (buffers + cached))
                               / total);
	}

	const char *
	ram_total(void)
	{
		uintmax_t total;

		if (pscanf("/proc/meminfo", "MemTotal: %ju kB\n", &total)
		    != 1) {
			return NULL;
		}

		return fmt_human(total * 1024, 1024);
	}

	const char *
	ram_used(void)
	{
    uintmax_t total, freem, buffers, cached, slab_reclaimable;

    FILE * fp;
    char * line = NULL;
    size_t len = 0;

    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
      return 0;

    int i = 0;
    while (getline(&line, &len, fp) != -1 && i <= 23) {
      switch (i) {
        case 0:
          sscanf(line, "MemTotal: %ju kB\n", &total);
          break;
        case 1:
          sscanf(line, "MemFree: %ju kB\n", &freem);
          break;
        case 3:
          sscanf(line, "Buffers: %ju kB\n", &buffers);
          break;
        case 4:
          sscanf(line, "Cached: %ju kB\n", &cached);
          break;
        case 23:
          sscanf(line, "SReclaimable: %ju kB\n", &slab_reclaimable);
          break;
      } 
        
      i++;
    }

    if (line)
      free(line);

    fclose(fp);

    // https://gitlab.com/procps-ng/procps/-/blob/master/proc/sysinfo.c#L781
    cached += slab_reclaimable;

    // https://gitlab.com/procps-ng/procps/-/blob/master/proc/sysinfo.c#L789
		return fmt_human((total - freem - buffers - cached) * 1024, 1024);
	}
#elif defined(__OpenBSD__)
	#include <stdlib.h>
	#include <sys/sysctl.h>
	#include <sys/types.h>
	#include <unistd.h>

	#define LOG1024 10
	#define pagetok(size, pageshift) (size_t)(size << (pageshift - LOG1024))

	inline int
	load_uvmexp(struct uvmexp *uvmexp)
	{
		int uvmexp_mib[] = {CTL_VM, VM_UVMEXP};
		size_t size;

		size = sizeof(*uvmexp);

		if (sysctl(uvmexp_mib, 2, uvmexp, &size, NULL, 0) >= 0) {
			return 1;
		}

		return 0;
	}

	const char *
	ram_free(void)
	{
		struct uvmexp uvmexp;
		int free_pages;

		if (load_uvmexp(&uvmexp)) {
			free_pages = uvmexp.npages - uvmexp.active;
			return fmt_human(pagetok(free_pages, uvmexp.pageshift) *
			                 1024, 1024);
		}

		return NULL;
	}

	const char *
	ram_perc(void)
	{
		struct uvmexp uvmexp;
		int percent;

		if (load_uvmexp(&uvmexp)) {
			percent = uvmexp.active * 100 / uvmexp.npages;
			return bprintf("%d", percent);
		}

		return NULL;
	}

	const char *
	ram_total(void)
	{
		struct uvmexp uvmexp;

		if (load_uvmexp(&uvmexp)) {
			return fmt_human(pagetok(uvmexp.npages,
			                         uvmexp.pageshift) * 1024,
			                 1024);
		}

		return NULL;
	}

	const char *
	ram_used(void)
	{
		struct uvmexp uvmexp;

		if (load_uvmexp(&uvmexp)) {
			return fmt_human(pagetok(uvmexp.active,
			                         uvmexp.pageshift) * 1024,
			                 1024);
		}

		return NULL;
	}
#elif defined(__FreeBSD__)
	#include <sys/sysctl.h>
	#include <sys/vmmeter.h>
	#include <unistd.h>
	#include <vm/vm_param.h>

	const char *
	ram_free(void) {
		struct vmtotal vm_stats;
		int mib[] = {CTL_VM, VM_TOTAL};
		size_t len;

		len = sizeof(struct vmtotal);
		if (sysctl(mib, 2, &vm_stats, &len, NULL, 0) == -1
				|| !len)
			return NULL;

		return fmt_human(vm_stats.t_free * getpagesize(), 1024);
	}

	const char *
	ram_total(void) {
		long npages;
		size_t len;

		len = sizeof(npages);
		if (sysctlbyname("vm.stats.vm.v_page_count", &npages, &len, NULL, 0) == -1
				|| !len)
			return NULL;

		return fmt_human(npages * getpagesize(), 1024);
	}

	const char *
	ram_perc(void) {
		long npages;
		long active;
		size_t len;

		len = sizeof(npages);
		if (sysctlbyname("vm.stats.vm.v_page_count", &npages, &len, NULL, 0) == -1
				|| !len)
			return NULL;

		if (sysctlbyname("vm.stats.vm.v_active_count", &active, &len, NULL, 0) == -1
				|| !len)
			return NULL;

		return bprintf("%d", active * 100 / npages);
	}

	const char *
	ram_used(void) {
		long active;
		size_t len;

		len = sizeof(active);
		if (sysctlbyname("vm.stats.vm.v_active_count", &active, &len, NULL, 0) == -1
				|| !len)
			return NULL;

		return fmt_human(active * getpagesize(), 1024);
	}
#endif
