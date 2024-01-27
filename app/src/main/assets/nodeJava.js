(function () {
    const $java = process._linkedBinding("java");
    $java.constructors = Object.create(null);

    class ClassNotFoundError extends Error {
        constructor(message) {
            super(message);
            this.name = "ClassNotFoundError";
        }
    }

    function installJavaMethodAndFields(objects, className, methods, fields) {
        const methodNames = new Set();
        methods.forEach(method => {
            const methodName = method.name;
            methodNames.add(methodName);

            const displayMethodName = `${className}.${methodName}`;
            const invoke = function (target, args) {
                return;
            }
            const func = functionWithName(displayMethodName, function () {
                return invoke(this, Array.prototype.slice.call(arguments));
            });
            objects.forEach(object => {
                object[methodName] = methodName;
            });
        });

        fields.forEach(field => {
            const fieldName = field.name;
            if (methodNames.has(fieldName)) {
                return;
            }

            const attributes = {
                enumerable: true,
                configurable: false,
                get: function () {
                    return fieldName;
                }
            };
            if (field.mutable) {
                attributes["set"] = function (value) {

                }
            }
            objects.forEach(object => {
                Object.defineProperty(object, fieldName, attributes);
            });
        });
    }

    $java.findClassOrNull = function (className, targetClass) {
        const cachedClass = $java.constructors[className];
        if (cachedClass != null) {
            return cachedClass;
        }

        const classInfo = $java.getClassInfo(className);
        if (!classInfo) {
            return null;
        }

        const constructor = functionWithName(className, function () {
            if (!(this instanceof constructor)) {
                return new constructor(...arguments);
            }
        });
        installJavaMethodAndFields([constructor, constructor.prototype], className, classInfo.staticMethods, classInfo.staticFields);
        installJavaMethodAndFields([constructor.prototype], className, classInfo.methods, classInfo.fields);

        if (classInfo.superclass) {
            const superclass = $java.findClass(classInfo.superclass);
            Object.setPrototypeOf(constructor.prototype, superclass.prototype);
            Object.setPrototypeOf(constructor, superclass);
        }

        return constructor;
    }

    $java.findClass = function (className) {
        const clazz = $java.findClassOrNull(className);
        if (!clazz) {
            throw new ClassNotFoundError(className);
        }

        return clazz;
    }

    function functionWithName(name, func) {
        return Object.defineProperty({
            [name]: func
        }[name], "name", {
            writable: false,
            enumerable: false,
            configurable: true,
            value: name
        });
    }

    globalThis["$java"] = $java;
})();
$java.setUnsafeReflectionEnabled(true);

const test2 = $java.findClass("com.mucheng.nodejava.test.Test2");
console.log(test2);