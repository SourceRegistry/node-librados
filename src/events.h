#pragma once

#include "common.h"

#include <atomic>

namespace nl {

class ObjectWatchWrap : public Napi::ObjectWrap<ObjectWatchWrap> {
 public:
  struct Data;
  static Napi::FunctionReference constructor;
  static void Init(Napi::Env env, Napi::Object exports);
  static Napi::Object Create(Napi::Env env, std::shared_ptr<IoState> io,
                             std::shared_ptr<ClusterState> cluster,
                             std::string oid, uint32_t timeout,
                             uint32_t queue_size, Napi::Function callback);
  explicit ObjectWatchWrap(const Napi::CallbackInfo& info);
  ~ObjectWatchWrap() override;

 private:
  Napi::Value Close(const Napi::CallbackInfo& info);
  Napi::Value Ack(const Napi::CallbackInfo& info);
  Napi::Value Cookie(const Napi::CallbackInfo& info);
  std::shared_ptr<Data> data_;
};

class MonitorLogWrap : public Napi::ObjectWrap<MonitorLogWrap> {
 public:
  struct Data;
  static Napi::FunctionReference constructor;
  static void Init(Napi::Env env, Napi::Object exports);
  static Napi::Object Create(Napi::Env env, std::shared_ptr<ClusterState> cluster,
                             std::string level, uint32_t queue_size,
                             Napi::Function callback);
  explicit MonitorLogWrap(const Napi::CallbackInfo& info);
  ~MonitorLogWrap() override;

 private:
  Napi::Value Close(const Napi::CallbackInfo& info);
  std::shared_ptr<Data> data_;
};

class ImageUpdateWatchWrap : public Napi::ObjectWrap<ImageUpdateWatchWrap> {
 public:
  struct Data;
  static Napi::FunctionReference constructor;
  static void Init(Napi::Env env, Napi::Object exports);
  static Napi::Object Create(Napi::Env env, std::shared_ptr<ImageState> image,
                             uint32_t queue_size, Napi::Function callback);
  explicit ImageUpdateWatchWrap(const Napi::CallbackInfo& info);
  ~ImageUpdateWatchWrap() override;

 private:
  Napi::Value Close(const Napi::CallbackInfo& info);
  std::shared_ptr<Data> data_;
};

}  // namespace nl
