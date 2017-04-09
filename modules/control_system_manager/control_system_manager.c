#include <control_system_manager.h>
#include <string.h>

/** Call a filter() pointer :
 * - lock the interrupts
 * - read the pointer to the filter function
 * - unlock the interrupts
 * - if pointer is null, return the IN value
 * - else apply filter
 */
static inline float
safe_filter(float (*f)(void *, float), void * param, float value)
{
	float (*f_tmp)(void *, float);
	void * param_tmp;
	f_tmp = f;
	param_tmp = param;
	if (f_tmp) {
		return f_tmp(param_tmp, value);
	}
	return value;
}

/** Call a processout() pointer :
 * - lock the interrupts
 * - read the pointer to the processout function
 * - unlock the interrupts
 * - if pointer is null, return 0
 * - else return the value processed by the function
 */
static inline float
safe_getprocessout(float (*f)(void *), void * param)
{
	float (*f_tmp)(void *);
	void * param_tmp;
    f_tmp = f;
	param_tmp = param;
	if (f_tmp) {
		return f_tmp(param_tmp);
	}
	return 0;
}

/** Call a processin() pointer :
 * - lock the interrupts
 * - read the pointer to the processin function
 * - unlock the interrupts
 * - if pointer is null, don't do anything
 * - else call the processin with the parameters
 */
static inline void
safe_setprocessin(void (*f)(void *, float), void * param, float value)
{
	void (*f_tmp)(void *, float);
	void * param_tmp;
    f_tmp = f;
	param_tmp = param;
	if (f_tmp) {
		f_tmp(param_tmp, value);
	}
}

/**********************************************/

void cs_init(struct cs* cs)
{
    memset(cs, 0, sizeof(struct cs));
    cs->enabled = 1;
}


void cs_set_consign_filter(struct cs* cs, float (*consign_filter)(void*, float), void* consign_filter_params)
{
    cs->consign_filter = consign_filter;
    cs->consign_filter_params = consign_filter_params;
}



void cs_set_correct_filter(struct cs* cs, float (*correct_filter)(void*, float), void* correct_filter_params)
{
    cs->correct_filter = correct_filter;
    cs->correct_filter_params = correct_filter_params;
}


void cs_set_feedback_filter(struct cs* cs, float (*feedback_filter)(void*, float), void* feedback_filter_params)
{
    cs->feedback_filter = feedback_filter;
    cs->feedback_filter_params = feedback_filter_params;
}

/** Set the cs output_filter fields in the cs structure */
void  cs_set_output_filter(struct cs* cs, float (*output_filter)(void*, float), void* output_filter_params) {
    cs->output_filter = output_filter;
    cs->output_filter_params = output_filter_params;
}


void cs_set_process_in(struct cs* cs, void (*process_in)(void*, float), void* process_in_params)
{
    cs->process_in = process_in;
    cs->process_in_params = process_in_params;
}



void cs_set_process_out(struct cs* cs, float (*process_out)(void*), void* process_out_params)
{
    cs->process_out = process_out;
    cs->process_out_params = process_out_params;
}



float cs_do_process(struct cs* cs, float consign)
{
    float process_out_value = 0;

    if (cs->enabled) {
        cs->consign_value = consign;

        cs->filtered_consign_value = safe_filter(cs->consign_filter, cs->consign_filter_params, consign);

        process_out_value = safe_getprocessout(cs->process_out, cs->process_out_params);
        process_out_value = safe_filter(cs->feedback_filter, cs->feedback_filter_params,
                                        process_out_value);
        cs->filtered_feedback_value = process_out_value;

        cs->error_value = cs->filtered_consign_value - process_out_value ;

        cs->out_value = safe_filter(cs->correct_filter, cs->correct_filter_params, cs->error_value);
        cs->out_value = safe_filter(cs->output_filter, cs->output_filter_params, cs->out_value);
    } else {
        cs->out_value = 0; /* disables the cs */
    }

    /* send out_value to process in*/
    safe_setprocessin (cs->process_in, cs->process_in_params, cs->out_value);

    /* return the out value */
    return (cs->out_value);
}



void cs_manage(void * data)
{
    struct cs* cs = data;
    cs_do_process(cs, cs->consign_value);
}



float cs_get_out(struct cs* cs)
{
   return cs->out_value;
}

float cs_get_error(struct cs* cs)
{
    return cs->error_value;
}



float cs_get_consign(struct cs* cs)
{
    return cs->consign_value;
}

float cs_get_filtered_consign(struct cs* cs)
{
    return cs->filtered_consign_value;
}

float cs_get_filtered_feedback(struct cs* cs)
{
    return cs->filtered_feedback_value;
}

float cs_get_feedback(struct cs* cs)
{
	return safe_getprocessout(cs->process_out, cs->process_out_params);
}

void cs_set_consign(struct cs* cs, float v)
{
    cs->consign_value = v;
}

void cs_enable(struct cs * cs)
{
    cs->enabled = 1;
}

void cs_disable(struct cs * cs)
{
    cs->enabled = 0;
}


