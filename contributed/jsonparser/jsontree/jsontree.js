// Copyright 2023 Mark Wharton (mark@jynx.com)
// https://opensource.org/license/mit/

import { NodeType, VPT } from "jsonparser";

class JSONTreeNode extends Array {
    // TODO: review
    // text;
    // type;

    constructor(type) {
        super();
        this.type = type;
    }

    get value() {
        switch (this.type) {
            case NodeType.null:
                return null;
            case NodeType.false:
                return false;
            case NodeType.true:
                return true;
            case NodeType.number:
                return this.text.includes(".") ? parseFloat(this.text) : parseInt(this.text, 10);
            case NodeType.string:
                return this.text.valueOf(); // TODO: review
            case NodeType.array: {
                let value = [];
                this.forEach(node => {
                    value.push(node.value);
                });
                return value;
            }
            case NodeType.object: {
                let value = {};
                this.forEach(node => {
                    // assert NodeType.field
                    value[node.text] = node[0].value;
                });
                return value;
            }
            case NodeType.field:
                return;
            case NodeType.root:
                return this[0] ? this[0].value : undefined;
        }
    }
}

class JSONTree extends VPT {
    // TODO: review
    // stack;

    constructor(keys) {
        super(keys);
        this.stack = [];
    }

    get root() {
        return this.node.value;
    }

    makeNode(nodeType) {
        return new JSONTreeNode(nodeType);
    }

    pop(nodeType) {
        if (this.node.type !== nodeType)
            return false;
        let node = this.stack.pop();
        // fields are pushed before the name is known, so we have to check and balance the tree
        // prune node that was rejected because it failed to match any of the keys
        if (this.node.length === 0 && this.node.text === undefined) {
            let index = node.indexOf(this.node);
            node.splice(index, 1);
        }
        this.node = node;
        return true;
    }

    push(nodeType) {
        let node = this.makeNode(nodeType);
        this.stack.push(this.node);
        this.node.push(node);
        this.node = node;
    }

    setText(text) {
        this.node.text = text;
    }
}

export { JSONTree, JSONTreeNode };
