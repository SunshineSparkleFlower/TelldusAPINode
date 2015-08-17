// hello.cc
#include <iostream>
#include <node.h>
#include <telldus-core.h>
#include <unistd.h>
#include <uv.h>
#include <string.h>
#include <stdlib.h>

using namespace v8;
using namespace std;

static uv_loop_t *loop;

static uv_async_t async;
static uv_mutex_t async_lock;

class Sensor_context {
private:
	void *context;
	char *protocol, *model, *value, *data, *newValue;
	int id, dataType, callbackId, timestamp, controllerId, deviceId, method;
	int changeEvent, changeType;

	void (Sensor_context::*sensor_callback)(void);
public:
	void *node_callback;

	Sensor_context(void *node_callback)
	{
		this->node_callback = node_callback;
	}

	Sensor_context(void) {}

	~Sensor_context(void) {}

	void set_triggerSensorEvent(void)
	{
		this->sensor_callback = &Sensor_context::triggerSensorEvent;
	}

	void set_triggerRawDeviceEvent(void)
	{
		this->sensor_callback = &Sensor_context::triggerRawDeviceEvent;
	}


	void set_triggerDeviceEvent(void)
	{
		this->sensor_callback = &Sensor_context::triggerDeviceEvent;
	}

	void set_triggerDeviceChangeEvent(void)
	{
		this->sensor_callback = &Sensor_context::triggerDeviceChangeEvent;
	}

	void set_triggerControllerEvent(void)
	{
		this->sensor_callback = &Sensor_context::triggerControllerEvent;
	}

	void trigger_event(void)
	{
		(this->*sensor_callback)();
	}

	void free_sensor_strings(void)
	{
		free(this->protocol);
		free(this->model);
		free(this->value);
	}

	void free_raw_strings(void)
	{
		free(this->data);
	}

	void free_device_data(void)
	{
		free(this->data);
	}

	void free_controller_data(void)
	{
		free(this->newValue);
	}

	void set_sensor_data(const char *protocol, const char *model, int id,
			int dataType, const char *value, int timestamp,
			int callbackId, void  *context)
	{
		this->protocol = strdup(protocol);
		this->model = strdup(model);
		this->value = strdup(value);
		this->id = id;
		this->dataType = dataType;
		this->timestamp = timestamp;
		this->callbackId = callbackId;
		this->context = context;
	}

	void set_raw_data(const char *data, int controllerId, int callbackId, void *context)
	{
		this->data = strdup(data);
		this->controllerId = controllerId;
		this->callbackId = callbackId;
		this->context = context;
	}

	void set_device_data(int deviceId, int method, const char *data,
			int callbackId, void *context)
	{
		this->deviceId = deviceId;
		this->method = method;
		this->data = strdup(data);
		this->callbackId = callbackId;
		this->context = context;
	}

	void set_device_change_data(int deviceId, int changeEvent,
			int changeType, int callbackId, void *context)
	{
		this->deviceId = deviceId;
		this->changeEvent = changeEvent;
		this->changeType = changeType;
		this->callbackId = callbackId;
		this->context = context;
	}

	void set_controller_data(int controllerId, int changeEvent,
			int changeType, const char *newValue,
			int callbackId, void *context)
	{
		this->controllerId = controllerId;
		this->changeEvent = changeEvent;
		this->changeType = changeType;
		this->newValue = strdup(newValue);
		this->callbackId = callbackId;
		this->context = context;
	}

	void triggerSensorEvent(void)
	{
		Isolate* isolate = Isolate::GetCurrent();
		HandleScope scope(isolate);

		const unsigned argc = 7;
		Local<Value> argv[argc] = {
			String::NewFromUtf8(isolate, this->protocol), 
			String::NewFromUtf8(isolate, this->model),
			Integer::New(isolate, this->id),
			Integer::New(isolate, this->dataType),
			String::NewFromUtf8(isolate, this->value), 
			Integer::New(isolate, this->timestamp),
			Integer::New(isolate, this->callbackId),
		};

		Persistent<Function, CopyablePersistentTraits<Function> > *cb =
			(Persistent<Function, CopyablePersistentTraits<Function> > *)this->node_callback;
		Local<Function> callback = Local<Function>::New(isolate, *cb);
		callback->Call(isolate->GetCurrentContext()->Global(), argc, argv);

		this->free_sensor_strings();
		uv_mutex_unlock(&async_lock);
		/*
		   delete ctx;
		   cb->Reset();
		   uv_close((uv_handle_t*)&async, NULL);
		   */
	}

	void triggerRawDeviceEvent(void)
	{
		Isolate* isolate = Isolate::GetCurrent();
		HandleScope scope(isolate);

		const unsigned argc = 3;
		Local<Value> argv[argc] = {
			String::NewFromUtf8(isolate, this->data), 
			Integer::New(isolate, this->controllerId),
			Integer::New(isolate, this->callbackId),
		};

		Persistent<Function, CopyablePersistentTraits<Function> > *cb =
			(Persistent<Function, CopyablePersistentTraits<Function> > *)this->node_callback;
		Local<Function> callback = Local<Function>::New(isolate, *cb);
		callback->Call(isolate->GetCurrentContext()->Global(), argc, argv);

		this->free_raw_strings();
		uv_mutex_unlock(&async_lock);
	}


	void triggerDeviceEvent(void)
	{
		Isolate* isolate = Isolate::GetCurrent();
		HandleScope scope(isolate);

		const unsigned argc = 4;
		Local<Value> argv[argc] = {
			Integer::New(isolate, this->deviceId),
			Integer::New(isolate, this->method),
			String::NewFromUtf8(isolate, this->data), 
			Integer::New(isolate, this->callbackId),
		};

		Persistent<Function, CopyablePersistentTraits<Function> > *cb =
			(Persistent<Function, CopyablePersistentTraits<Function> > *)this->node_callback;
		Local<Function> callback = Local<Function>::New(isolate, *cb);
		callback->Call(isolate->GetCurrentContext()->Global(), argc, argv);

		this->free_device_data();
		uv_mutex_unlock(&async_lock);
	}

	void triggerDeviceChangeEvent(void)
	{
		Isolate* isolate = Isolate::GetCurrent();
		HandleScope scope(isolate);

		const unsigned argc = 4;
		Local<Value> argv[argc] = {
			Integer::New(isolate, this->deviceId),
			Integer::New(isolate, this->changeEvent),
			Integer::New(isolate, this->changeType),
			Integer::New(isolate, this->callbackId),
		};

		Persistent<Function, CopyablePersistentTraits<Function> > *cb =
			(Persistent<Function, CopyablePersistentTraits<Function> > *)this->node_callback;
		Local<Function> callback = Local<Function>::New(isolate, *cb);
		callback->Call(isolate->GetCurrentContext()->Global(), argc, argv);

		uv_mutex_unlock(&async_lock);
	}

	void triggerControllerEvent(void)
	{
		Isolate* isolate = Isolate::GetCurrent();
		HandleScope scope(isolate);

		const unsigned argc = 5;
		Local<Value> argv[argc] = {
			Integer::New(isolate, this->controllerId),
			Integer::New(isolate, this->changeEvent),
			Integer::New(isolate, this->changeType),
			String::NewFromUtf8(isolate, this->newValue), 
			Integer::New(isolate, this->callbackId),
		};

		Persistent<Function, CopyablePersistentTraits<Function> > *cb =
			(Persistent<Function, CopyablePersistentTraits<Function> > *)this->node_callback;
		Local<Function> callback = Local<Function>::New(isolate, *cb);
		callback->Call(isolate->GetCurrentContext()->Global(), argc, argv);

		this->free_controller_data();
		uv_mutex_unlock(&async_lock);
	}
};

/*void _tdUnregisterCallback(const FunctionCallbackInfo<Value> &args)
{
	Local<Integer> id = Local<Integer>::Cast(args[0]);
	tdUnregisterCallback(id);
}
*/


void _tdGetNumberOfDevices(const FunctionCallbackInfo<Value>& args){
	int intNumberOfDevices = tdGetNumberOfDevices();
	args.GetReturnValue().Set(intNumberOfDevices);
}

void _tdclose(const FunctionCallbackInfo<Value>& args){
	tdClose();
}

void _tdTurnOn(const FunctionCallbackInfo<Value>& args){
	printf("id: %d\n", args[0]->Int32Value()); 	
	printf("turnOn: %d\n", tdTurnOn(args[0]->Int32Value())); 
}

void _tdTurnOff(const FunctionCallbackInfo<Value>& args){
	tdTurnOff(args[0]->Int32Value()); 
}

void sensor_event_callback(const char *protocol, const char *model, int id, int dataType,
		const char *value, int timestamp,
		int callbackId, void  *context){

	uv_mutex_lock(&async_lock);

	Sensor_context *ctx = (Sensor_context *)async.data;
	ctx->node_callback = context;
	ctx->set_triggerSensorEvent();
	ctx->set_sensor_data(protocol, model, id, dataType, value, timestamp,
			callbackId, context);

	uv_async_send(&async);
}

void raw_device_event_callback(const char *data, int controllerId,
		int callbackId, void *context)
{
	uv_mutex_lock(&async_lock);

	Sensor_context *ctx = (Sensor_context *)async.data;
	ctx->node_callback = context;
	ctx->set_triggerRawDeviceEvent();
	ctx->set_raw_data(data, controllerId, callbackId, context);

	uv_async_send(&async);
}

void device_event_callback(int deviceId, int method, const char *data,
		int callbackId, void *context)
{
	uv_mutex_lock(&async_lock);

	Sensor_context *ctx = (Sensor_context *)async.data;
	ctx->node_callback = context;
	ctx->set_triggerDeviceEvent();
	ctx->set_device_data(deviceId, method, data, callbackId, context);

	uv_async_send(&async);

}

void device_change_event_callback(int deviceId, int changeEvent,
		int changeType, int callbackId, void *context)
{
	uv_mutex_lock(&async_lock);

	Sensor_context *ctx = (Sensor_context *)async.data;
	ctx->node_callback = context;
	ctx->set_triggerDeviceChangeEvent();
	ctx->set_device_change_data(deviceId, changeEvent, changeType,
			callbackId, context);

	uv_async_send(&async);
}

void controller_event(int controllerId, int changeEvent,
		int changeType, const char *newValue,
		int callbackId, void *context)
{
	uv_mutex_lock(&async_lock);

	Sensor_context *ctx = (Sensor_context *)async.data;
	ctx->node_callback = context;
	ctx->set_triggerControllerEvent();
	ctx->set_controller_data(controllerId, changeEvent, changeEvent,
			newValue, callbackId, context);

	uv_async_send(&async);
}

void trigger_node_event(uv_async_t *handle)
{
	Sensor_context *ctx = (Sensor_context *)handle->data;
	ctx->trigger_event();
}

void _tdRegisterSensorEvent(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);

	v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(args[0]);

	void *test = new Persistent<Function,
	     CopyablePersistentTraits<Function> >(isolate, cb);

	tdRegisterSensorEvent(sensor_event_callback, test);
}
void _tdRegisterRawDeviceEvent(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);

	v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(args[0]);

	void *test = new Persistent<Function,
	     CopyablePersistentTraits<Function> >(isolate, cb);

	tdRegisterRawDeviceEvent(raw_device_event_callback, test);
}

void init(Handle<Object> exports) {
	NODE_SET_METHOD(exports, "tdclose", _tdclose);
	NODE_SET_METHOD(exports, "tdTurnOff", _tdTurnOff);
	NODE_SET_METHOD(exports, "tdTurnOn", _tdTurnOn);
	NODE_SET_METHOD(exports, "tdGetNumberOfDevices", _tdGetNumberOfDevices);
	NODE_SET_METHOD(exports, "tdRegisterSensorEvent", _tdRegisterSensorEvent);
	NODE_SET_METHOD(exports, "tdRegisterRawDeviceEvent", _tdRegisterRawDeviceEvent);
//	NODE_SET_METHOD(exports, "tdUnregisterCallback", _tdUnregisterCallback);
//	NODE_SET_METHOD(exports, "", );


	tdInit();

	loop = uv_default_loop();	
	uv_mutex_init(&async_lock);

	Sensor_context *ctx = new Sensor_context();
	uv_async_init(loop, &async, trigger_node_event);	

	async.data = ctx;
}

NODE_MODULE(addon, init)
