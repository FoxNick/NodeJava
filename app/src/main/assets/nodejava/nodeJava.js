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

    function installJavaMethodAndFields(object, className, methods, fields) {
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
            object[methodName] = func;
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
            Object.defineProperty(object, fieldName, attributes);
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

        const isGenerated = classInfo.isGenerated;
        const constructor = functionWithName(className, function () {
            if (!(this instanceof constructor)) {
                return new constructor(...arguments);
            }

            if (arguments.length === 1 && arguments[0] === lateInitSymbol) {
                return;
            }

            const args = Array.prototype.slice.call(arguments);
            if (isGenerated) {
                args.push($java.__wrapAsInterface(this));
            }

            const javaObjectId = $java.__createJavaObject(className, args);
            $java.__makeReference(this, javaObjectId);
        });

        installJavaMethodAndFields(constructor, className, classInfo.staticMethods, classInfo.staticFields);
        installJavaMethodAndFields(constructor.prototype, className, classInfo.methods, classInfo.fields);

        if (classInfo.superclass) {
            const superclass = $java.findClass(classInfo.superclass);
            Object.setPrototypeOf(constructor.prototype, superclass.prototype);
        }

        installInnerClasses(constructor, classInfo.classes);
        Object.defineProperty(constructor, "class", {
            get: function() {
                return $java.classForName(className);
            },
            set: () => false
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

    async function _defineClass(className, superclass, interfaces, methods, outputDexFile) {
        if ($java.findClassOrNull(className)) {
            throw new Error(`defineClass with a existing class: ${className}`);
        }

        console.log(className, superclass, interfaces, methods, outputDexFile);

        $java.__defineClass(className, superclass, interfaces, methods, outputDexFile);
        return $java.findClass(className);
    }

    $java.defineClass = async function (jsClass, options) {
        if (!jsClass.name) {
            throw new Error(`class does not has name: ${jsClass}`);
        }
        const className = (options?.packageName ? options.packageName + "." : "") + jsClass.name;

        let clazz = jsClass;
        const methods = new Set();
        while (true) {
            const proto = clazz?.prototype;
            if (!clazz || !proto) {
                throw new Error(`not a class or class does not extend java class: ${jsClass}`);
            }
            if (clazz.hasOwnProperty($java.className)) {
                break;
            }
            Object.getOwnPropertyNames(proto)
                .filter(property => typeof proto[property])
                .forEach(method => methods.add(method));;
            clazz = Object.getPrototypeOf(clazz);
        }
        methods.delete("constructor");

        const path = require("path");
        const fs = require("fs/promises");

        const interfaceNames = (options?.implements || []).map(it => it.class.getName());
        const superClassName = clazz[$java.className];
        let outputDexFile = options?.cacheDexFile;
        if (!outputDexFile) {
            const name = superClassName + ":" + [, ...interfaceNames].join(',') + ";methods:" + [...methods].join(',');
            const md5 = require("crypto").createHash("md5").update(name).digest("hex");
            outputDexFile = path.join(process.cwd(), ".codecache", `${className}_${md5}_v1.dex`);
        }
        let exists;
        try {
            await fs.access(outputDexFile);
            exists = true;
        } catch (e) {
            exists = false;
        }
        let generatedClass;
        if (false) {
            $java.loadDex(outputDexFile);
            generatedClass = $java.findClass(className);
        } else {
            await fs.mkdir(path.dirname(outputDexFile), { recursive: true });
            generatedClass = await _defineClass(className, superClassName, interfaceNames, [...methods], outputDexFile);
        }
        Object.setPrototypeOf(jsClass, generatedClass);
        Object.setPrototypeOf(jsClass.prototype, generatedClass.prototype);
        return jsClass;
    }

    const resolve = require("path").resolve;
    $java.loadDex = function (dex) {
        $java.__loadDex(resolve(dex));
    }

    globalThis["$java"] = $java;
})();