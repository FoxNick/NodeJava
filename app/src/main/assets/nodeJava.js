(function () {
    const $java = process._linkedBinding("java");
    $java.constructors = Object.create(null);

    const lateInitSymbol = Symbol("lateInit");

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
                return $java.getReturnValue($java.__callMethod(className, methodName, args, target));
            }
            const func = functionWithName(displayMethodName, function () {
                return invoke(this, Array.prototype.slice.call(arguments));
            });
            objects.forEach(object => {
                object[methodName] = func;
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
                    return $java.getReturnValue($java.__getField(className, fieldName, this));
                }
            };
            if (field.mutable) {
                attributes["set"] = function (value) {
                    $java.__setField(className, fieldName, value, this);
                }
            }
            objects.forEach(object => {
                Object.defineProperty(object, fieldName, attributes);
            });
        });
    }

    $java.getReturnValue = function (returnValue) {
        const javaClass = returnValue.javaClass;
        if (javaClass == null) {
            return returnValue.value;
        }
        if (javaClass == "array") {
            return returnValue.value.map(e => $java.getReturnValue(e));
        }
        const clazz = $java.findClass(javaClass);
        const result = new clazz(lateInitSymbol);
        $java.__makeReference(result, returnValue.value);

        return result;
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

            const args = Array.prototype.slice.call(arguments);
            if (args.length === 1 && arguments[0] === lateInitSymbol) {
                return;
            }

            $java.__makeReference(this, $java.__createJavaObject(className, args));
        });
        installJavaMethodAndFields([constructor, constructor.prototype], className, classInfo.staticMethods, classInfo.staticFields);
        installJavaMethodAndFields([constructor.prototype], className, classInfo.methods, classInfo.fields);

        if (classInfo.superclass) {
            const superclass = $java.findClass(classInfo.superclass);
            Object.setPrototypeOf(constructor.prototype, superclass.prototype);
            Object.setPrototypeOf(constructor, superclass);
        }

        installInnerClasses(constructor, classInfo.classes);
        Object.defineProperty(constructor, "class", {
            get: function() {
                return $java.classForName(className);
            }
        });

        constructor[$java.className] = className;
        return constructor;
    }

    $java.classForName = function (className) {
        return $java.getReturnValue($java.__classForName(className));
    }

    $java.findClass = function (className) {
        const clazz = $java.findClassOrNull(className);
        if (!clazz) {
            throw new ClassNotFoundError(className);
        }

        return clazz;
    }

    function installInnerClasses(object, declaredClasses) {
        declaredClasses.forEach(declaredClassName => {
            const dollar = declaredClassName.lastIndexOf("$");
            if (dollar < 0 || dollar >= declaredClassName.length - 1) {
                return;
            }
            try {
                const declaredClass = $java.findClassOrNull(declaredClassName);
                if (declaredClass) {
                    const simplifiedName = declaredClassName.substring(dollar + 1);
                    object[simplifiedName] = declaredClass
                }
            } catch (e) {

            }
        });
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

const test2 = $java.findClass("com.mucheng.nodejava.test.Test");
test2.w = 50;
console.log(test2.class);