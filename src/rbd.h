#pragma once

#include "common.h"

namespace nl {

class RbdPoolWrap : public Napi::ObjectWrap<RbdPoolWrap> {
 public:
  static Napi::FunctionReference constructor;
  static void Init(Napi::Env env, Napi::Object exports);
  static Napi::Object NewInstance(Napi::Env env, std::shared_ptr<IoState> io,
                                  std::shared_ptr<ClusterState> cluster,
                                  std::string pool);
  explicit RbdPoolWrap(const Napi::CallbackInfo& info);

 private:
  Napi::Value List(const Napi::CallbackInfo& info);
  Napi::Value Create(const Napi::CallbackInfo& info);
  Napi::Value Remove(const Napi::CallbackInfo& info);
  Napi::Value Rename(const Napi::CallbackInfo& info);
  Napi::Value Open(const Napi::CallbackInfo& info);
  Napi::Value Clone(const Napi::CallbackInfo& info);
  Napi::Value TrashMove(const Napi::CallbackInfo& info);
  Napi::Value TrashRestore(const Napi::CallbackInfo& info);
  Napi::Value TrashRemove(const Napi::CallbackInfo& info);
  Napi::Value NamespaceList(const Napi::CallbackInfo& info);
  Napi::Value NamespaceCreate(const Napi::CallbackInfo& info);
  Napi::Value NamespaceRemove(const Napi::CallbackInfo& info);
  Napi::Value PoolStats(const Napi::CallbackInfo& info);
  Napi::Value Advanced(const Napi::CallbackInfo& info);

  std::shared_ptr<IoState> io_;
  std::shared_ptr<ClusterState> cluster_;
  std::string pool_;
};

class RbdImageWrap : public Napi::ObjectWrap<RbdImageWrap> {
 public:
  static Napi::FunctionReference constructor;
  static void Init(Napi::Env env, Napi::Object exports);
  static Napi::Object NewInstance(Napi::Env env, std::shared_ptr<ImageState> image,
                                  std::shared_ptr<IoState> io, std::string name);
  explicit RbdImageWrap(const Napi::CallbackInfo& info);

 private:
  Napi::Value Close(const Napi::CallbackInfo& info);
  Napi::Value IsClosed(const Napi::CallbackInfo& info);
  Napi::Value Stat(const Napi::CallbackInfo& info);
  Napi::Value Size(const Napi::CallbackInfo& info);
  Napi::Value Resize(const Napi::CallbackInfo& info);
  Napi::Value Features(const Napi::CallbackInfo& info);
  Napi::Value Flags(const Napi::CallbackInfo& info);
  Napi::Value Read(const Napi::CallbackInfo& info);
  Napi::Value Write(const Napi::CallbackInfo& info);
  Napi::Value Discard(const Napi::CallbackInfo& info);
  Napi::Value Flush(const Napi::CallbackInfo& info);
  Napi::Value InvalidateCache(const Napi::CallbackInfo& info);
  Napi::Value SnapCreate(const Napi::CallbackInfo& info);
  Napi::Value SnapRemove(const Napi::CallbackInfo& info);
  Napi::Value SnapList(const Napi::CallbackInfo& info);
  Napi::Value SnapSet(const Napi::CallbackInfo& info);
  Napi::Value SnapProtect(const Napi::CallbackInfo& info);
  Napi::Value SnapUnprotect(const Napi::CallbackInfo& info);
  Napi::Value SnapRollback(const Napi::CallbackInfo& info);
  Napi::Value Flatten(const Napi::CallbackInfo& info);
  Napi::Value MetadataGet(const Napi::CallbackInfo& info);
  Napi::Value MetadataSet(const Napi::CallbackInfo& info);
  Napi::Value MetadataRemove(const Napi::CallbackInfo& info);
  Napi::Value MetadataList(const Napi::CallbackInfo& info);
  Napi::Value LockAcquire(const Napi::CallbackInfo& info);
  Napi::Value LockRelease(const Napi::CallbackInfo& info);
  Napi::Value LockBreak(const Napi::CallbackInfo& info);
  Napi::Value LockOwner(const Napi::CallbackInfo& info);
  Napi::Value WatchUpdates(const Napi::CallbackInfo& info);
  Napi::Value Advanced(const Napi::CallbackInfo& info);

  std::shared_ptr<ImageState> image_;
  std::shared_ptr<IoState> io_;
  std::string name_;
};

}  // namespace nl
