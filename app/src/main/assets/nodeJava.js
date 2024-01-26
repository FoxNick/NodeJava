(function() {
    const $java = process._linkedBinding("java");
    $java.constructors = {};
    $java.prototypes = {};

    class ClassNotFoundError extends Error {
        constructor(message) {
            super(message);
            this.name = "ClassNotFoundError";
        }
    }

    $java.findClassOrNull = function (className) {
        const cachedClass = $java.constructors[className];
        if (typeof cachedClass !== "undefined") {
            return cachedClass;
        }

        const classInfo = $java.getClassInfo(className);
        if (!classInfo) {
            return null;
        }

        const newClass = createJavaConstructor(classInfo);
        $java.constructors[className] = newClass;

        const prototype = newClass.prototype;
        installJavaMethodAndFields(prototype, className, classInfo.methods, classInfo.fields);
        if (classInfo.superclass) {
            const superclass = $java.findClassOrNull(classInfo.superclass);
            Object.setPrototypeOf(prototype, superclass.prototype);
        }

        $java.prototypes[className] = prototype;

        installInnerClasses(newClass, classInfo.declaredClasses);

        const javaClass = lazy(() => $java.classForName(className));
        Object.defineProperty(newClass, "class", {
            get: javaClass,
            set: () => false
        });

        return newClass;
    }

    $java.findClass = function (className) {
        const clazz = $java.findClassOrNull(className);
        if (!clazz) {
            throw new ClassNotFoundError(className);
        }
        return clazz;
    }

    $java.classForName = function (className) {
        return $java.getReturnValue($java.__classForName(className));
    }

    function createJavaConstructor(classInfo) {
        const constructor = function () {
            if (classInfo.isArray) {
                return constructJavaArray(classInfo, arguments, this, constructor);
            }

            const args = Array.prototype.slice.call(arguments);
            const javaObjectRef = $java.__createJavaObject(classInfo.className, args);
            $java.__makeReference(this, javaObjectRef);
        }
        constructor[$java.className] = classInfo.className;
        installJavaMethodAndFields(constructor, classInfo.className, classInfo.staticMethods, classInfo.staticFields);
        return constructor;
    }

    function installJavaMethodAndFields(object, className, methods, fields) {
        const methodNames = new Set();
        methods.forEach(method => {
            const methodName = method.name;
            methodNames.add(methodName);

            const invoke = function (target, args) {
                return $java.getReturnValue($java.callMethod(target, method.name, args));
            }
            const func = function () {
                const args = Array.prototype.slice.call(arguments);
                return invoke(this, args);
            }
            func.invoke = invoke;
            object[methodName] = func;
        });

        fields.forEach(field => {
            if (methodNames.has(field.name)) {
                return;
            }
            const attributes = {
                enumerable: true,
                configurable: false,
                get: function () {
                    console.log("Run")
                    return $java.getReturnValue($java.getField(this, field.name));
                }
            };
            if (field.mutable) {
                attributes["set"] = function (value) {
                    $java.setField(this, field.name, value);
                }
            }
            Object.defineProperty(object, field.name, attributes);
        });
    }

    const lazyJavaArrayClass = lazy(() => $java.findClass("java.lang.reflect.Array"));
    function constructJavaArray(classInfo, args, target, constructor) {
        return proxyJavaArray(lazyJavaArrayClass().newInstance(constructor.class.getComponentType(), args[0]));
    }

    function proxyJavaArray(javaArray) {
        const javaArrayClass = lazyJavaArrayClass();
        if (typeof (javaArray.length) == "undefined") {
            let length = undefined;
            Object.defineProperty(javaArray, "length", {
                get: () => {
                    if (typeof length == "undefined") {
                        length = javaArrayClass.getLength(javaArray);
                    }
                    return length;
                }
            });
        }
        return new Proxy(javaArray, {
            get: function(target, key, receiver) {
                if (typeof key == "string") {
                    const n = Math.floor(Number(key));
                    if (n !== Infinity && String(n) === key && n >= 0) {
                        return javaArrayClass.get(javaArray, n);
                    }
                }
                return Reflection.get(...arguments);
            },
            set: function(target, key, receiver) {
                if (typeof key == "string") {
                    const n = Math.floor(Number(key));
                    if (n !== Infinity && String(n) === key && n >= 0) {
                        javaArrayClass.set(javaArray, n, value);
                    }
                }
                return Reflection.get(...arguments);
            }
        });
    }

    function lazy(evaluator) {
        let value;
        let hasValue = false;
        return () => {
            if (hasValue) {
                return value;
            }
            value = evaluator();
            hasValue = true;
            return value;
        };
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
                    object[simplifiedName] = declaredClass;
                }
            } catch (e) {

            }
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
        const result = new clazz();

        return result;
    }

    globalThis["$java"] = $java;
})();

$java.setUnsafeReflectionEnabled(true);

const clazz = $java.findClass("com.mucheng.nodejava.test.Test");
const instance = new clazz();
console.log(instance.a);
console.log(instance.b);
instance.print("hello");
instance.print(1);