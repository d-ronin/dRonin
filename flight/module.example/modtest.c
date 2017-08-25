#include <loadable_extension.h>

static void entry();

DECLARE_LOADABLE_EXTENSION(entry);

#include <stddef.h>
#include <pios_thread.h>
#include <pios_modules.h>

static volatile int i = 3333;
volatile int j;

//void dumb_test_delay(int i) __attribute__((long_call));
void dumb_test_delay(int i);
void dumb_test_two(void (*fp)(void));

void dumb_regtask(struct pios_thread *h);

const char *const foo = "foobar";
/* const char *foo = "test"; doesn't work presently.
 * GOT correctly ends up pointing to offset on copydata.
 * But offset on copydata points to location requiring a relocation.
 * analysis here:
 * https://gist.github.com/mlyle/308393dbc7df3762a366d585d5306ceb
 */

static void loopy(void *unused) {
	(void) unused;

	while (1) {
		PIOS_Thread_Sleep(10);
		dumb_test_delay(i);
		dumb_test_delay(j);
		system_annunc_custom_string(foo);
	}

}

static void entry() {
	j = i * 2;

	struct pios_thread *loadable_task_handle;

	loadable_task_handle = PIOS_Thread_Create(loopy, "module", PIOS_THREAD_STACK_SIZE_MIN, NULL, PIOS_THREAD_PRIO_NORMAL);
	if (loadable_task_handle) {
		dumb_regtask(loadable_task_handle);
	}
}
