// Just a scratchpad for ideas...

template<typename T, int N=1024>
class GenericSystem : GoSystem {
private:
	Bitset<N> freeMask;
	T slots[N];

public:
	
	public GenericSystem(GoComponentType type) : GoSystem(type) {
		freeMask.mark();
	}
	
	virtual int onInit(GoComponent *component, const void *args) {
		// allocate slot
		unsigned index;
		if (!freeMask.clearFirst(index)) {
			return -1;
		}
		// create bidirecional map
		setComData(component, slots+index);
		slots[index]->genericInit(component);
		return slots[index]->Init(args);
	}

	virtual int onEnable(GoComponent *component) {
		T* inst = (T*) comData(component);
		return inst->Enable();
	}

	virtual int onDisable(GoComponent *component) {
		T* inst = (T*) comData(component);
		return inst->Disable();

	}

	virtual int onDestroy(GoComponent *component) {
		T* inst = (T*) comData(component);
		inst->Destroy();
		freeMask.mark(inst - slots);
	}

	// iterator?

};

class GenericComponent {
private:
	GoComponent *mComponent;

public:
	GoComponentRef component() { return mComponent; }
	GameObjectRef gameObject() { return component().gameObject(); }

	template<typename T>
	T getComponent(GoComponentType type) { return gameObject().getComponent(type).data<T>(); }

	void genericInit(GoComponent *aComponent) {
		mComponent = aComponent;
	}
};

class EditComponent : GenericComponent {
private:
	NodeRef node;
	bool expanded;
	// UNDO stack?

public:
	
	int Init(const void* args=0) {
		node = getComponent<Node>(LP_COMPONENT_TYPE_NODE);
		expanded = true;
	}

	int Enable() {}
	int Disable() {}
	int Destroy() {}
	

};

class EditSystem : GenericSystem<EditComponent> {

	EditComponent *getEdit(GameObjectRef go) {
		auto result = go.getComponent(comType());
		if (!result) {
			result = go.addComponent(comType());
		}
		return result.data<EditComponent>();
	}

	void drawTreeView(SpritePlotter *plotter);
};

