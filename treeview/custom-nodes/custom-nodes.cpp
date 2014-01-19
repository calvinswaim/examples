/* Copyright (c) 2014  Niklas Rosenstein
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * file: treeview-example.cpp
 * description: cinema 4d plugin demonstrating the tree view GUI with
 *   a custom tree node class. written for the cinema 4d R15 api. */

#include <c4d.h>
#include <customgui_listview.h>

static const Int32 PLUGIN_ID = 1031676;

class BaseNode {

protected:

    BaseNode* m_next;
    BaseNode* m_pred;
    BaseNode* m_up;
    BaseNode* m_down;

public:

    BaseNode() : m_next(nullptr), m_pred(nullptr), m_up(nullptr), m_down(nullptr) { }

    virtual ~BaseNode() {
        m_next = m_pred = m_up = m_down = nullptr;
    }

    BaseNode* GetNext() const {
        return m_next;
    }

    BaseNode* GetPred() const {
        return m_pred;
    }

    BaseNode* GetUp() const {
        return m_up;
    }

    BaseNode* GetDown() const {
        return m_down;
    }

    BaseNode* GetDownLast() const {
        BaseNode* child = this->m_down;
        while (child && child->m_next)
            child = child->m_next;
        return child;
    }

    void InsertAfter(BaseNode* pred) {
        if (pred->m_next) {
            pred->m_next->m_pred = this;
            this->m_next = pred->m_next;
        }
        pred->m_next = this;
        this->m_pred = pred;

        if (pred->m_up)
            this->m_up = m_pred->m_up;
    }

    void InsertBefore(BaseNode* next) {
        if (next->m_pred) {
            next->m_pred->m_next = this;
            this->m_pred = next->m_pred;
        }
        next->m_pred = this;
        this->m_next = next;

        if (next->m_up)
            this->m_up = next->m_up;
        if (this->m_pred == nullptr && this->m_up)
            this->m_up->m_down = this;
    }

    void InsertUnder(BaseNode* up) {
        if (up->m_down) {
            this->InsertBefore(up->m_down);
        }
        else {
            up->m_down = this;
            this->m_up = up;
        }
    }

    void InsertUnderLast(BaseNode* up) {
        if (up->m_down) {
            this->InsertAfter(up->GetDownLast());
        }
        else {
            this->InsertUnder(up);
        }
    }

    void Remove() {
        if (this->m_up) {
            if (this->m_pred == nullptr)
                DebugAssert(this->m_up->m_down == this);
            if (this->m_up->m_down == this) {
                this->m_up->m_down = this->m_next;
            }
        }
        if (this->m_next) {
            this->m_next->m_pred = this->m_pred;
        }
        if (this->m_pred) {
            this->m_pred->m_next = this->m_next;
        }

        this->m_up = this->m_pred = this->m_next = nullptr;
    }

};

class Node : public BaseNode {

    typedef BaseNode super;

protected:

    virtual void CopyFrom(const Node* other) {
        name = other->name;
        opened = other->opened;
        selected = other->opened;
    }

    virtual Node* AllocShallowCopy() const {
        return new Node;
    }

public:

    String name;
    Bool opened, selected;

    Node() : super(), name(""), opened(true), selected(false) { }

    Node(const String& name) : super(), name(name), opened(true), selected(false) { }

    virtual ~Node() {
    }

    Node* GetNext() const {
        return static_cast<Node*>(m_next);
    }

    Node* GetPred() const {
        return static_cast<Node*>(m_pred);
    }

    Node* GetUp() const {
        return static_cast<Node*>(m_up);
    }

    Node* GetDown() const {
        return static_cast<Node*>(m_down);
    }

    Node* GetLinNext() const {
        if (m_down)
            return GetDown();

        Node* parent = GetUp();
        while (parent && parent->GetNext() == nullptr)
            parent = parent->GetUp();

        if (parent)
            return parent->GetNext();
        return nullptr;
    }

    /* Returns the selected node if there is only one or nullptr. */
    Node* FindSelected() {
        Node* result = nullptr;
        Node* current = this;
        while (current) {

            if (current->selected) {
                if (result)
                    return nullptr;
                result = current;
            }

            current = current->GetLinNext();
        }

        return result;
    }

    /* Set the selection state of all *child* nodes. */
    void SelectAll(Bool state) const {
        Node* child = GetDown();
        while (child) {
            child->selected = state;
            child->SelectAll(state);
            child = child->GetNext();
        }
    }

    /* Create a copy of the node. */
    Node* CreateCopy(Bool children=true) const {
        Node* copy = AllocShallowCopy();
        if (copy == nullptr)
            return nullptr;

        copy->CopyFrom(this);

        Bool success = true;
        if (children) {
            Node* current = GetDown();
            while (current) {
                Node* current_copy = CreateCopy(true);
                if (current_copy == nullptr) {
                    success = false;
                    break;
                }
                current_copy->InsertUnderLast(copy);
                current = current->GetNext();
            }
        }

        if (!success) {
            Node::FreeTree(copy);
            delete copy;
            return nullptr;
        }

        return copy;
    }

    static void FreeTree(Node* root) {
        Node* current = root->GetDown();
        while (current) {
            Node* next = current->GetNext();
            FreeTree(current);
            current->Remove();
            delete current;
            current = next;
        }
    }

    static void FreeSelectedNodes(Node* root) {
        if (root->selected) {
            root->Remove();
            FreeTree(root);
            delete root;
        }
        Node* current = root->GetDown();
        while (current) {
            Node* next = current->GetNext();
            FreeSelectedNodes(current);
            current = next;
        }
    }

};

class TreeModel : public TreeViewFunctions {

public:

    virtual void* GetFirst(void* root, void* ud) {
        if (root)
            return static_cast<Node*>(root)->GetDown();
        else
            return nullptr;
    }

    virtual void* GetNext(void* root, void* ud, void* obj) {
        return static_cast<Node*>(obj)->GetNext();
    }

    virtual void* GetPred(void* root, void* ud, void* obj) {
        return static_cast<Node*>(obj)->GetPred();
    }

    virtual void* GetUp(void* root, void* ud, void* obj) {
        return static_cast<Node*>(obj)->GetUp();
    }

    virtual void* GetDown(void* root, void* ud, void* obj) {
        return static_cast<Node*>(obj)->GetDown();
    }

    virtual Bool IsSelected(void* root, void* ud, void* obj) {
        return static_cast<Node*>(obj)->selected;
    }

    virtual void Select(void* root, void* ud, void* obj, Int32 mode) {
        Node* node = static_cast<Node*>(obj);
        switch (mode) {
            case SELECTION_NEW:
                static_cast<Node*>(root)->SelectAll(false);
            case SELECTION_ADD:
                if (node) node->selected = true;
                break;
            case SELECTION_SUB:
                if (node) node->selected = false;
                break;
        }
    }

    virtual Bool IsOpened(void* root, void* ud, void* obj) {
        return static_cast<Node*>(obj)->opened;
    }

    virtual void Open(void* root, void* ud, void* obj, Bool opened) {
        static_cast<Node*>(obj)->opened = opened;
    }

    virtual String GetName(void* root, void* ud, void* obj) {
        return static_cast<Node*>(obj)->name;
    }

    virtual void SetName(void* root, void* ud, void* obj, const String& name) {
        static_cast<Node*>(obj)->name = name;
    }

    virtual Int GetId(void* root, void* ud, void* obj) {
        return (Int) obj;
    }

    virtual Int32 GetDragType(void* root, void* ud, void* obj) {
        // Just return a unique ID here if you don't use one of the
        // default DRAGTYPEs.
        return PLUGIN_ID;
    }

    virtual Int32 AcceptDragObject(void* root, void* userdata, void* obj,
                Int32 type, void* drag_ptr, Bool& allow_copy) {
        if (type != PLUGIN_ID)
            return 0;

        allow_copy = true;
        return INSERT_AFTER | INSERT_UNDER | INSERT_BEFORE;
    }

    virtual void InsertObject(void* root, void* userdata, void* obj,
                Int32 type, void* drag_ptr, Int32 mode, Bool copy) {
        if (type != PLUGIN_ID)
            return;

        Node* source = static_cast<Node*>(drag_ptr);
        if (copy) {
            source = source->CreateCopy();
            if (source == nullptr)
                return; // memory error
        }

        Node* dest = static_cast<Node*>(obj);
        if (!dest) {
            source->InsertUnderLast(static_cast<Node*>(root));
        }
        else {
            switch (mode) {
                case INSERT_UNDER:
                    source->Remove();
                    source->InsertUnder(dest);
                    break;
                case INSERT_BEFORE:
                    source->Remove();
                    source->InsertBefore(dest);
                    break;
                case INSERT_AFTER:
                    source->Remove();
                    source->InsertAfter(dest);
                    break;
                default:
                    // invalid type, deallocate the source again.
                    if (copy) delete source;
                    source = nullptr;
                    break;
            }
        }
    }

    virtual void DeletePressed(void* root, void* userdata) {
        // Delete all selected nodes.
        Node::FreeSelectedNodes(static_cast<Node*>(root));
    }

};

class Dialog : public GeDialog {

    typedef GeDialog super;

    Node m_root;
    TreeModel m_model;
    TreeViewCustomGui* m_tree;

    enum {
        BUTTON_ADD = 10000,
        TREEVIEW,
        FULLFIT = BFH_SCALEFIT | BFV_SCALEFIT,
    };

public:

    Dialog() : super(), m_root("<root>"), m_model(), m_tree(nullptr) { }

    virtual ~Dialog() {
        Node::FreeTree(&m_root);
    }

    void AddNode() {
        DebugAssert(m_tree != nullptr);

        // Create the new node.
        Node* new_node = new Node("New Node");
        if (new_node == nullptr) {
            MessageDialog("memory error");
            return;
        }

        // Find the selected node or insert it as the last child of
        // the root.
        Node* selected = m_root.FindSelected();
        if (selected)
            new_node->InsertAfter(selected);
        else
            new_node->InsertUnderLast(&m_root);

        // Deselect all other nodes and select the new one.
        m_root.SelectAll(false);
        new_node->selected = true;

        // Refresh the tree.
        m_tree->Refresh();
    }

    /*( GeDialog )*/
    /*( ======== )*/

    virtual Bool CreateLayout() {
        if (GroupBeginInMenuLine()) {
            AddButton(BUTTON_ADD, 0, 0, 0, "+");
            GroupEnd();
        }
        else return false;

        if (GroupBegin(0, FULLFIT, 0, 0, "", 0)) {
            BaseContainer data;
            m_tree = static_cast<TreeViewCustomGui*>(AddCustomGui(
                    TREEVIEW, CUSTOMGUI_TREEVIEW, "", BFH_SCALEFIT | BFV_SCALEFIT,
                    0, 0, data));
            GroupEnd();

            if (m_tree == nullptr)
                return false;
        }
        else return false;
        return true;
    }

    virtual Bool InitValues() {
        if (m_tree == nullptr)
            return false;

        m_tree->SetRoot(&m_root, &m_model, nullptr);
        return true;
    }

    virtual Bool Command(Int32 id, const BaseContainer& msg) {
        switch (id) {
            case BUTTON_ADD:
                AddNode();
                break;
        }
        return true;
    }

};

class Command : public CommandData {

    Dialog m_dialog;

public:

    /*( CommandData )*/
    /*( =========== )*/

    virtual Bool Execute(BaseDocument* doc) {
        return m_dialog.Open(DLG_TYPE_ASYNC, PLUGIN_ID, -2, -2, 500, 300);
    }

};

Bool PluginStart() {
    return RegisterCommandPlugin(PLUGIN_ID, "TreeView Test", 0, nullptr, "", NewObj(Command));
}

Bool PluginMessage(Int32 type, void* pData) {
    return true;
}

void PluginEnd() {
}

