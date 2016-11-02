#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <artik_module.h>
#include <artik_loop.h>
#include <artik_cloud.h>

#define CHECK_RET(x)	{ if (x != S_OK) goto exit; }
#define TEST_TIMEOUT_MS	(30*1000)
#define MAX_PARAM_LEN 128

char access_token[MAX_PARAM_LEN];
char device_id[MAX_PARAM_LEN];
char sdr_access_token[MAX_PARAM_LEN];
char sdr_device_id[MAX_PARAM_LEN];
char *test_message = NULL;
char *sdr_test_message = NULL;

static void on_timeout_callback(void *user_data)
{
	artik_loop_module *loop = (artik_loop_module *) user_data;

	fprintf(stdout, "TEST: %s stop scanning, exiting loop\n", __func__);

	loop->quit();
}

void websocket_receive_callback(void *user_data, void *result)
{
	char *buffer = (char *)result;
	if (buffer == NULL) {
		fprintf(stdout, "receive failed\n");
		return;
	}
	printf("received: %s\n", buffer);
	free(result);
}

static artik_error test_websocket_read(int timeout_ms)
{
	artik_error ret = S_OK;
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	artik_loop_module *loop = (artik_loop_module *)artik_request_api_module("loop");

	artik_websocket_handle handle;
	int timeout_id = 0;

	/* Open websocket to ARTIK Cloud and register device to receive messages from cloud */
	ret = cloud->websocket_open_stream(&handle, access_token, device_id, false);
	if (ret != S_OK) {
		fprintf(stderr, "TEST failed, could not open Websocket (%d)\n", ret);
		return ret;
	}

	ret = cloud->websocket_set_receive_callback(handle, websocket_receive_callback, &handle);
	if (ret != S_OK) {
		fprintf(stderr, "TEST failed, could not open Websocket (%d)\n", ret);
		return ret;
	}

	ret = loop->add_timeout_callback(&timeout_id, timeout_ms, on_timeout_callback,
					   (void *)loop);

	loop->run();

	cloud->websocket_close_stream(handle);

	fprintf(stdout, "TEST: %s finished\n", __func__);

	artik_release_api_module(cloud);
	artik_release_api_module(loop);

	return ret;
}

void websocket_receive_write_callback(void *user_data, void *result)
{
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	char *buffer = (char *)result;
	if (buffer == NULL) {
		fprintf(stdout, "receive failed\n");
		return;
	}
	printf("received: %s\n", buffer);
	free(result);

	cloud->websocket_send_message(*(artik_websocket_handle *)user_data, test_message);
}

static artik_error test_websocket_write(int timeout_ms)
{
	artik_error ret = S_OK;
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	artik_loop_module *loop = (artik_loop_module *)artik_request_api_module("loop");

	artik_websocket_handle handle;
	int timeout_id = 0;

	/* Open websocket to ARTIK Cloud and register device to receive message from cloud */
	ret = cloud->websocket_open_stream(&handle, access_token, device_id, false);
	if (ret != S_OK) {
		fprintf(stderr, "TEST failed, could not open Websocket (%d)\n", ret);
		return ret;
	}

	ret = cloud->websocket_set_receive_callback(handle, websocket_receive_write_callback, &handle);
	if (ret != S_OK) {
		fprintf(stderr, "TEST failed, could not open Websocket (%d)\n", ret);
		return ret;
	}

	ret = loop->add_timeout_callback(&timeout_id, timeout_ms, on_timeout_callback,
					   (void *)loop);

	loop->run();

	cloud->websocket_close_stream(handle);

	fprintf(stdout, "TEST: %s finished\n", __func__);

	artik_release_api_module(cloud);
	artik_release_api_module(loop);

	return ret;
}

void websocket_sdr_receive_write_callback(void *user_data, void *result)
{
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	char *buffer = (char *)result;
	if (buffer == NULL) {
		fprintf(stdout, "receive failed\n");
		return;
	}
	printf("received: %s\n", buffer);
	free(result);

	cloud->websocket_send_message(*(artik_websocket_handle *)user_data, sdr_test_message);
}

static artik_error test_websocket_sdr(int timeout_ms)
{
	artik_error ret = S_OK;
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	artik_loop_module *loop = (artik_loop_module *)artik_request_api_module("loop");

	artik_websocket_handle handle;
	int timeout_id = 0;

	/* Open websocket to ARTIK Cloud and register device to receive message from cloud */
	ret = cloud->websocket_open_stream(&handle, sdr_access_token, sdr_device_id, true);
	if (ret != S_OK) {
		fprintf(stderr, "TEST failed, could not open Websocket (%d)\n", ret);
		return ret;
	}

	ret = cloud->websocket_set_receive_callback(handle, websocket_sdr_receive_write_callback, &handle);
	if (ret != S_OK) {
		fprintf(stderr, "TEST failed, could not open Websocket (%d)\n", ret);
		return ret;
	}

	ret = loop->add_timeout_callback(&timeout_id, timeout_ms, on_timeout_callback,
					   (void *)loop);

	loop->run();

	cloud->websocket_close_stream(handle);

	fprintf(stdout, "TEST: %s finished\n", __func__);

	artik_release_api_module(cloud);
	artik_release_api_module(loop);

	return ret;
}

int main(int argc, char *argv[])
{
	int opt;
	artik_error ret = S_OK;

	while ((opt = getopt(argc, argv, "t:d:a:s:m:g:")) != -1) {
		switch (opt) {
		case 't':
			strncpy(access_token, optarg, MAX_PARAM_LEN);
			break;
		case 'd':
			strncpy(device_id, optarg, MAX_PARAM_LEN);
			break;
		case 'a':
			strncpy(sdr_access_token, optarg, MAX_PARAM_LEN);
			break;
		case 's':
			strncpy(sdr_device_id, optarg, MAX_PARAM_LEN);
			break;
		case 'm':
			test_message = strndup(optarg, strlen(optarg));
			break;
		case 'g':
			sdr_test_message = strndup(optarg, strlen(optarg));
			break;
		default:
			printf("Usage: websocket-test [-t <access token>] [-d <device id>] [-a <SDR access token>] \r\n");
			printf("\t[-s <SDR device ID>] [-m <JSON type test message>] [-g <JSON type SDR test message>] \r\n");
			return 0;
		}
	}

	ret = test_websocket_sdr(TEST_TIMEOUT_MS);
	CHECK_RET(ret);

	ret = test_websocket_write(TEST_TIMEOUT_MS);
	CHECK_RET(ret);

	ret = test_websocket_read(TEST_TIMEOUT_MS);
	CHECK_RET(ret);

exit:
	if (test_message != NULL)
		free(test_message);
	if (sdr_test_message != NULL)
		free(sdr_test_message);

	return (ret == S_OK) ? 0 : -1;
}
