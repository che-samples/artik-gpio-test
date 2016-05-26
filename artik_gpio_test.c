#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <artik_module.h>
#include <artik_platform.h>
enum {
    R = 0,
    G,
    B
};
struct led_gpios{
    artik_gpio_handle handle;
    artik_gpio_config config;
};
static artik_error test_rgb_led(int platid)
{
    artik_gpio_module* gpio = (artik_gpio_module*)artik_get_api_module("gpio");
    unsigned int i;
    artik_error ret;
    struct led_gpios leds[] = {
        {NULL, {ARTIK_A5_GPIO_XEINT0, "red", GPIO_OUT, GPIO_DIGITAL, GPIO_EDGE_NONE, 0 }},      /* Connected to J26-2 */
        {NULL, {ARTIK_A5_GPIO_XEINT1, "green", GPIO_OUT, GPIO_DIGITAL, GPIO_EDGE_NONE, 0 }},        /* Connected to J26-3 */
        {NULL, {ARTIK_A5_GPIO_XEINT2, "blue", GPIO_OUT, GPIO_DIGITAL, GPIO_EDGE_NONE, 0 }},     /* Connected to J26-4 */
    };
    if(platid == ARTIK5) {
        leds[0].config.id = ARTIK_A5_GPIO_XEINT0;
        leds[1].config.id = ARTIK_A5_GPIO_XEINT1;
        leds[2].config.id = ARTIK_A5_GPIO_XEINT2;
    } else {
        leds[0].config.id = ARTIK_A10_GPIO_XEINT0;
        leds[1].config.id = ARTIK_A10_GPIO_XEINT1;
        leds[2].config.id = ARTIK_A10_GPIO_XEINT2;
    }
    fprintf(stdout, "TEST: %s\n", __func__);
    /* Register GPIOs for LEDs */
    for (i=0; i<sizeof(leds)/sizeof(*leds); i++) {
        ret = gpio->request(&leds[i].handle, &leds[i].config);
        if (ret != S_OK)
            return ret;
    }
    /* Play around with all possible colors */
    gpio->write(leds[R].handle, 1);  // R
    usleep(1000*1000);
    gpio->write(leds[G].handle, 1);  // RG
    usleep(1000*1000);
    gpio->write(leds[B].handle, 1); // RGB
    usleep(1000*1000);
    gpio->write(leds[R].handle, 0);  // GB
    usleep(1000*1000);
    gpio->write(leds[B].handle, 0);  // G
    usleep(1000*1000);
    gpio->write(leds[G].handle, 0);
    gpio->write(leds[B].handle, 1);  // B
    usleep(1000*1000);
    gpio->write(leds[R].handle, 1);  // RB
    usleep(1000*1000);
    /* Release GPIOs for LEDs */
    for (i=0; i<sizeof(leds)/sizeof(*leds); i++) {
        gpio->release(leds[i].handle);
    }
    fprintf(stdout, "TEST: %s succeeded\n", __func__);
    return ret;
}
static void* button_event(void* param)
{
    artik_gpio_module* gpio = (artik_gpio_module*)artik_get_api_module("gpio");
    artik_gpio_handle button = (artik_gpio_handle)param;
    int state, new_state;
    state = gpio->read(button);
    while(true) {
        new_state = gpio->wait_for_change(button);
        if (new_state < 0) {
            fprintf(stdout, "Wait for change returned %d\n", new_state);
            break;
        }
        fprintf(stdout, "Button event: %d -> %d\n", state, new_state);
        state = new_state;
    }
    return NULL;
}
static artik_error test_button_interrupt(int platid)
{
    artik_gpio_module* gpio = (artik_gpio_module*)artik_get_api_module("gpio");
    artik_error ret = S_OK;
    pthread_t button_thread;
    artik_gpio_handle button;
    artik_gpio_config config;
    if(platid == ARTIK5)
        config.id = ARTIK_A5_GPIO_XEINT3;
    else
        config.id = ARTIK_A10_GPIO_XEINT3;
    config.name = "button";
    config.dir = GPIO_IN;
    config.type = GPIO_DIGITAL;
    config.edge = GPIO_EDGE_BOTH;
    config.initial_value = 0;
    fprintf(stdout, "TEST: %s\n", __func__);
    ret = gpio->request(&button, &config);
    if (ret != S_OK) {
        fprintf(stderr, "TEST: %s failed, could not request GPIO (%d)\n", __func__, ret);
        return ret;
    }
    /* Start event thread */
    pthread_create(&button_thread, NULL, button_event, button);
    /* Sleep for x seconds while events are handled in the thread */
    usleep(10*1000*1000);
    /* Cancel and join thread */
    pthread_cancel(button_thread);
    pthread_join(button_thread, NULL);
    gpio->release(button);
    fprintf(stdout, "TEST: %s succeeded\n", __func__);
    return ret;
}
int main()
{
    artik_error ret = S_OK;
    int platid = artik_get_platform();
    if((platid == ARTIK5) || (platid == ARTIK10)) {
        ret = test_button_interrupt(platid);
        ret = test_rgb_led(platid);
    }
    return (ret == S_OK) ? 0 : -1;
}
