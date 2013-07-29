#ifndef NODE_EXTENSION_H
#define NODE_EXTENSION_H

#include "xwalk_extension_public.h"

struct NodeExtensionContext;

struct NodeExtension : public CXWalkExtension {
private:
  static CXWalkExtensionContext* CreateContext(CXWalkExtension* self);
  static const char* GetJavascript(CXWalkExtension* self);
  static void Shutdown(CXWalkExtension* self);
public:
  NodeExtension();

  friend struct NodeExtensionContext;
};

struct NodeExtensionContext : public CXWalkExtensionContext {
private:
  NodeExtension *extension_;

  static void HandleMessage(CXWalkExtensionContext* self, const char* message);
  static void Destroy(CXWalkExtensionContext* self);

public:
  NodeExtensionContext(NodeExtension *extension);
};

#endif  // NODE_EXTENSION_H
