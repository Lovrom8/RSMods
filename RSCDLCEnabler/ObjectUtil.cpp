#include "ObjectUtil.hpp"

namespace ObjectUtil {
	Object* GetRootObject() {
		Object* root = (Object*)MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_rootObject, Offsets::ptr_rootObjectOffsets);

		return root;
	}

	std::vector<Object*> GetChildrenOfObject(Object* parent) {
		std::vector<Object*> children;

		if (parent == nullptr) return children;

		for (size_t i = 0; i < parent->childCount; i++)
		{
			if (parent->children[i] != nullptr) {
				children.push_back(parent->children[i]);
			}
		}

		return children;
	}
}