// Copyright 2023 Mark Wharton (mark@jynx.com)
// https://opensource.org/license/mit/

class JSONParser @ "xs_jsonparser_destructor" {
    constructor(vpt) @ "xs_jsonparser_constructor";
    initialize(initialStackDepth, initialBufferSize) @ "xs_jsonparser_initialize";
    receive(segment, start, end) @ "xs_jsonparser_receive";
    terminate() @ "xs_jsonparser_terminate";
}
JSONParser.failure = -1;
JSONParser.receive = 0;
JSONParser.success = 1;

class Matcher {
    // TODO: review
    // func;
    // init;
    // term;

    constructor(func = undefined, init = undefined, term = undefined) {
        this.func = func;
        this.init = init;
        this.term = term;
    }

    initialize(vpt) {
        if (this.init)
            this.init(vpt);
        return this;
    }

    match(vpt, node) {
        if (this.func)
            this.func(vpt, node);
    }

    terminate(vpt) {
        if (this.term)
            this.term(vpt);
    }
}

class Node {
    // TODO: review
    // $;
    // next;
    // prev;
    // text;
    // type;

    constructor(type, prev = undefined) {
        this.type = type;
        if (prev) {
            this.prev = prev;
            this.prev.next = this;
        }
    }

    get data() {
        if (this.$ === undefined)
            this.$ = {};
        return this.$;
    }

    up(count = 1, nodeType = undefined, nodeText = undefined) {
        let node = this;
        while (node && count--) {
            node = node.prev;
        }
        return (node && (!nodeType || (node.type === nodeType && (!nodeText || node.text === nodeText)))) ? node : undefined;
    }
}

const NodeType = Object.freeze({
    null: 0,
    false: 1,
    true: 2,
    number: 3,
    string: 4,
    array: 5,
    object: 6,
    field: 7,
    root: 8,
});

class VPT {
    // TODO: review
    // keys;
    // matcher;
    // node;

    constructor(keys = undefined, matcher = undefined) {
        this.keys = keys;
        this.matcher = matcher;
        this.node = this.makeNode(NodeType.root);
    }

    initialize() {
        return this.matcher ? this.matcher.initialize(this) : undefined;
    }

    makeNode(nodeType, prev = undefined) {
        return new Node(nodeType, prev);
    }

    pop(nodeType) {
        if (this.node.type !== nodeType)
            return false;
        if (this.matcher)
            this.matcher.match(this, this.node);
        this.node = this.node.prev;
        return true;
    }

    push(nodeType) {
        this.node = this.makeNode(nodeType, this.node);
    }

    setText(text) {
        this.node.text = text;
    }

    terminate() {
        if (this.matcher)
            this.matcher.terminate(this);
    }
}

export { JSONParser, Matcher, Node, NodeType, VPT };
