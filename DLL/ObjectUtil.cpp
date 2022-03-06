#include "ObjectUtil.hpp"

namespace ObjectUtil {
	Object* GetRootObject() {
		_LOG_INIT;
		_LOG_SETLEVEL(LogLevel::Error);

		Object* root = (Object*)MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_rootObject, Offsets::ptr_rootObjectOffsets, true);

		if (root == NULL)
			_LOG("(ObjUtil) Root object is NULL. Some Twitch mods may not work" << std::endl);

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

	void UpdateScales() {
		ObjectUtil::Object* rootObject = ObjectUtil::GetRootObject();
		std::vector<ObjectUtil::Object*> children = ObjectUtil::GetChildrenOfObject(rootObject);

		for (auto child : children)
		{
			std::string className = child->className;

			//If object class name is in object scale map
			if (ObjectScaleMap.find(className) == ObjectScaleMap.end()) continue;

			child->scale = ObjectScaleMap[className];
		}
	}

	void SetObjectScales(std::map<std::string, float> scales)
	{
		//Apply scales to map
		for (auto pair : scales) {
			ObjectScaleMap[pair.first] = pair.second;
		}
	}

	std::map<std::string, float> ObjectScaleMap{};

	std::vector<std::string> AllNoteParts = { "ActorChordLetter", "MeshBracketLarge", "MeshNoteBendArrow", "MeshNoteBendPanel", "MeshNoteIconFretHandMute", "MeshNoteIconHammerOn", "MeshNoteIconHarmonic", "MeshNoteIconPalmMute",
		"MeshNoteIconPinchHarmonic", "MeshNoteIconPop", "MeshNoteIconPullOff", "MeshNoteIconSlap", "MeshNoteIconTap", "MeshNoteOpenAccent", "MeshNoteOpenArpeggio", "MeshNoteOpenLH", "MeshNoteOpenRH", "MeshNoteSingleAccent",
		"MeshNoteSingleArpeggio", "MeshNoteSingleLH", "MeshNoteSingleRH", "MeshStemBracket", "MeshStemDoubleStop", "MeshStemNote", "MeshStemNoteOpen", "MeshStrumBracketLarge", "MeshStrumBracketSmall", "MeshStrumLarge", "MeshStrumLargeAccent",
		"MeshStrumLargeArpeggio", "MeshStrumLargeDoubleStop", "MeshStrumLargeFretMute", "MeshStrumLargePalmMute", "MeshStrumSmall", "MeshStrumSmallAccent", "MeshStrumSmallDoubleStop", "MeshStrumSmallDoubleStopFretMute",
	    "MeshStrumSmallDoubleStopPalmMute", "MeshStrumSmallFretMute", "MeshStrumSmallPalmMute", "NoteTail", "NoteTailMesh", "MeshFingerprintZoneLeft", "MeshFingerprintZoneRight" };
}