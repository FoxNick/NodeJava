Object.defineProperty(exports, "__esModule", { value: true });
exports.Packages = exports.install = undefined;

const packageProxy = {
    get(obj, prop) {
        if (typeof (prop) !== 'string') {
            return obj[prop];
        }
        return obj.getChild(prop);
    },
};

class JavaPackage {
    constructor(packageName) {
        this.packageName = packageName;
        this.cache = new Map();
        this.proxy = new Proxy(this, packageProxy);
    }
    getChild(name) {
        const cached = this.cache.get(name);
        if (typeof cached !== 'undefined') {
            return cached;
        }
        const clazz = this.findPublicOrInnerClass(name);
        if (clazz) {
            this.cache.set(name, clazz);
            return clazz;
        }
        const newPackage = new JavaPackage(this.getFullName(name, '.')).proxy;
        this.cache.set(name, newPackage);
        return newPackage;
    }
    findPublicOrInnerClass(className) {
        const publicClass = $java.findClassOrNull(this.getFullName(className, '.'));
        if (publicClass) {
            return publicClass;
        }
        const innerClass = $java.findClassOrNull(this.getFullName(className, '$'));
        return innerClass;
    }
    get [Symbol.toStringTag]() {
        if (this.packageName === undefined) {
            return '[object Packages]';
        }
        return `[JavaPackage ${this.packageName}]`;
    }
    inspect() {
        return this.toString();
    }
    getFullName(name, joiner) {
        const pkg = this.packageName;
        return pkg === undefined ? name : pkg + joiner + name;
    }
}

const topPackages = ["java", "javax", "org", "com", "edu", "net", "android", "androidx"];
class Rhino {
    constructor() {
        this.packages = new JavaPackage().proxy;
    }
    install(scope) {
        topPackages.forEach(pkg => {
            scope[pkg] = this.packages[pkg];
        });
        scope.Packages = this.packages;
    }
}

const rhino = new Rhino();
function install(scope = global) {
    rhino.install(scope);
}

exports.install = install;
exports.Packages = rhino.packages;