/****************************************************************************
**
** Copyright (C) 2014 Ford Motor Company
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "RepCodeGenerator.h"

#include "RepParser.h"

#ifndef QT_BOOTSTRAPPED
# define HAVE_QSAVEFILE
#endif
#ifdef HAVE_QSAVEFILE
# include <QSaveFile>
#else
# include <QFile>
#endif
#include <QTextStream>

template <typename C>
static int accumulatedSizeOfNames(const C &c)
{
    int result = 0;
    foreach (const typename C::value_type &e, c)
        result += e.name.size();
    return result;
}

template <typename C>
static int accumulatedSizeOfTypes(const C &c)
{
    int result = 0;
    foreach (const typename C::value_type &e, c)
        result += e.type.size();
    return result;
}

static QString cap(QString name)
{
    if (!name.isEmpty())
        name[0] = name[0].toUpper();
    return name;
}

RepCodeGenerator::RepCodeGenerator(const QString &fileName)
    : m_fileName(fileName)
{
}

bool RepCodeGenerator::generate(const AST &ast, Mode mode)
{
#ifdef HAVE_QSAVEFILE
    QSaveFile file(m_fileName);
#else
    QFile file(m_fileName);
#endif
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QString includeGuardName(m_fileName.toUpper());
    includeGuardName.replace('.', '_');

    QTextStream stream(&file);

    stream << "#ifndef " << includeGuardName << endl;
    stream << "#define " << includeGuardName << endl << endl;

    QStringList out;

    generateHeader(mode, stream, ast);
    foreach (const POD &pod, ast.pods)
        generatePOD(stream, pod);
    const QString podMetaTypeRegistrationCode = generateMetaTypeRegistrationForPODs(ast.pods);
    foreach (const ASTClass &astClass, ast.classes)
        generateClass(mode, out, astClass, podMetaTypeRegistrationCode);

    stream << out.join("\n");

    stream << endl << "#endif // " << includeGuardName << endl;

#ifdef HAVE_QSAVEFILE
    file.commit();
#endif
    return true;
}

void RepCodeGenerator::generateHeader(Mode mode, QTextStream &out, const AST &ast)
{
    out << "// This is an autogenerated file.\n"
           "// Do not edit this file, any changes made will be lost the next time it is generated.\n"
           "\n"
           "#include <QObject>\n"
           "#include <QVariantList>\n"
           "#include <QMetaProperty>\n"
           "\n"
           "#include <QRemoteObjectNode>\n"
           ;
    if (mode == REPLICA)
        out << "#include <qremoteobjectreplica.h>\n";
    else
        out << "#include <QRemoteObjectSource>\n";
    out << "\n";

    out << ast.includes.join(QLatin1Char('\n'));
    out << "\n";
}

static QString formatTemplateStringArgTypeNameCapitalizedName(int numberOfTypeOccurrences, int numberOfNameOccurrences,
                                                              QString templateString, const POD &pod)
{
    QString out;
    const int LengthOfPlaceholderText = 2;
    Q_ASSERT(templateString.count(QRegExp("%\\d")) == numberOfNameOccurrences + numberOfTypeOccurrences);
    const int expectedOutSize
            = numberOfNameOccurrences * accumulatedSizeOfNames(pod.attributes)
            + numberOfTypeOccurrences * accumulatedSizeOfTypes(pod.attributes)
            + pod.attributes.size() * (templateString.size() - (numberOfNameOccurrences + numberOfTypeOccurrences) * LengthOfPlaceholderText);
    out.reserve(expectedOutSize);
    foreach (const PODAttribute &a, pod.attributes)
        out += templateString.arg(a.type, a.name, cap(a.name));
    return out;
}

QString RepCodeGenerator::formatQPropertyDeclarations(const POD &pod)
{
    return formatTemplateStringArgTypeNameCapitalizedName(1, 4, QStringLiteral("    Q_PROPERTY(%1 %2 READ %2 WRITE set%3 NOTIFY on%3Changed)\n"), pod);
}

QString RepCodeGenerator::formatConstructors(const POD &pod)
{
    QString initializerString = QString("QObject(parent)");
    QString defaultInitializerString = initializerString;
    QString argString;
    foreach (const PODAttribute &a, pod.attributes) {
        initializerString += QString(", _%1(%1)").arg(a.name);
        defaultInitializerString += QString(", _%1()").arg(a.name);
        argString += QString("%1 %2, ").arg(a.type, a.name);
    }
    argString.chop(2);

    return QString("    explicit %1(QObject *parent = Q_NULLPTR) : %2 {}\n"
                   "    explicit %1(%3, QObject *parent = Q_NULLPTR) : %4 {}\n")
            .arg(pod.name, defaultInitializerString, argString, initializerString);
}

QString RepCodeGenerator::formatCopyConstructor(const POD &pod)
{
    return "    " + pod.name + "(const " + pod.name + "& other)\n"
           "        : QObject()\n"
           "    {\n"
           "        QtRemoteObjects::copyStoredProperties(&other, this);\n"
           "    }\n"
           "\n"
           ;
}

QString RepCodeGenerator::formatCopyAssignmentOperator(const POD &pod)
{
    return "    " + pod.name + " &operator=(const " + pod.name + " &other)\n"
           "    {\n"
           "        if (this != &other)\n"
           "            QtRemoteObjects::copyStoredProperties(&other, this);\n"
           "        return *this;\n"
           "    }\n"
           "\n"
           ;
}

QString RepCodeGenerator::formatPropertyGettersAndSetters(const POD &pod)
{
    // MSVC doesn't like adjacent string literal concatenation within QStringLiteral, so keep it in one line:
    QString templateString
            = QStringLiteral("    %1 %2() const { return _%2; }\n    void set%3(%1 %2) { if (%2 != _%2) { _%2 = %2; Q_EMIT on%3Changed(); } }\n");
    return formatTemplateStringArgTypeNameCapitalizedName(2, 9, qMove(templateString), pod);
}

QString RepCodeGenerator::formatSignals(const POD &pod)
{
    QString out;
    const QString prefix = QStringLiteral("    void on");
    const QString suffix = QStringLiteral("Changed();\n");
    const int expectedOutSize
            = accumulatedSizeOfNames(pod.attributes)
            + pod.attributes.size() * (prefix.size() + suffix.size());
    out.reserve(expectedOutSize);
    foreach (const PODAttribute &a, pod.attributes) {
        out += prefix;
        out += cap(a.name);
        out += suffix;
    }
    Q_ASSERT(out.size() == expectedOutSize);
    return out;
}

QString RepCodeGenerator::formatDataMembers(const POD &pod)
{
    QString out;
    const QString prefix = QStringLiteral("    ");
    const QString infix  = QStringLiteral(" _");
    const QString suffix = QStringLiteral(";\n");
    const int expectedOutSize
            = accumulatedSizeOfNames(pod.attributes)
            + accumulatedSizeOfTypes(pod.attributes)
            + pod.attributes.size() * (prefix.size() + infix.size() + suffix.size());
    out.reserve(expectedOutSize);
    foreach (const PODAttribute &a, pod.attributes) {
        out += prefix;
        out += a.type;
        out += infix;
        out += a.name;
        out += suffix;
    }
    Q_ASSERT(out.size() == expectedOutSize);
    return out;
}

QString RepCodeGenerator::formatMarshalingOperators(const POD &pod)
{
    return "inline QDataStream &operator<<(QDataStream &ds, const " + pod.name + " &obj) {\n"
           "    QtRemoteObjects::copyStoredProperties(&obj, ds);\n"
           "    return ds;\n"
           "}\n"
           "\n"
           "inline QDataStream &operator>>(QDataStream &ds, " + pod.name + " &obj) {\n"
           "    QtRemoteObjects::copyStoredProperties(ds, &obj);\n"
           "    return ds;\n"
           "}\n"
           ;
}

void RepCodeGenerator::generatePOD(QTextStream &out, const POD &pod)
{
    out << "class " << pod.name << " : public QObject\n"
           "{\n"
           "    Q_OBJECT\n"
        <<      formatQPropertyDeclarations(pod)
        << "public:\n"
        <<      formatConstructors(pod)
        <<      formatCopyConstructor(pod)
        <<      formatCopyAssignmentOperator(pod)
        <<      formatPropertyGettersAndSetters(pod)
        << "Q_SIGNALS:\n"
        <<      formatSignals(pod)
        << "private:\n"
        <<      formatDataMembers(pod)
        << "};\n"
           "\n"
        << formatMarshalingOperators(pod)
        << "\n"
           "Q_DECLARE_METATYPE(" << pod.name << ")\n"
           "\n"
           ;
}

QString RepCodeGenerator::generateMetaTypeRegistrationForPODs(const QVector<POD> &pods)
{
    QString out;
    const QString qRegisterMetaType = QStringLiteral("        qRegisterMetaType<");
    const QString qRegisterMetaTypeStreamOperators = QStringLiteral("        qRegisterMetaTypeStreamOperators<");
    const QString lineEnding = QStringLiteral(">();\n");
    const int expectedOutSize
            = 2 * accumulatedSizeOfNames(pods)
            + pods.size() * (qRegisterMetaType.size() + qRegisterMetaTypeStreamOperators.size() + 2 * lineEnding.size());
    out.reserve(expectedOutSize);
    foreach (const POD &pod, pods) {
        out += qRegisterMetaType;
        out += pod.name;
        out += lineEnding;

        out += qRegisterMetaTypeStreamOperators;
        out += pod.name;
        out += lineEnding;
    }
    Q_ASSERT(expectedOutSize == out.size());
    return out;
}

void RepCodeGenerator::generateClass(Mode mode, QStringList &out, const ASTClass &astClass, const QString &podMetaTypeRegistrationCode)
{
    const QString className = (astClass.name() + (mode == REPLICA ? "Replica" : "Source"));
    if (mode == REPLICA)
        out << QString("class %1 : public QRemoteObjectReplica").arg(className);
    else
        out << QString("class %1 : public QRemoteObjectSource").arg(className);

    out << "{";
    out << "    Q_OBJECT";
    out << QString("    Q_CLASSINFO(QCLASSINFO_REMOTEOBJECT_TYPE, \"%1\")").arg(astClass.name());
    out << QString("    friend class QRemoteObjectNode;");
    if (mode == SOURCE)
        out <<"public:";
    else
        out <<"private:";

    if (mode == REPLICA) {
        out << QString("    %1() : QRemoteObjectReplica() {}").arg(className);
        out << QString("    void initialize()");
    } else {
        out << QString("    %1(QObject *parent = Q_NULLPTR) : QRemoteObjectSource(parent)").arg(className);
    }

    out << "    {";

    if (!podMetaTypeRegistrationCode.isEmpty())
        out << podMetaTypeRegistrationCode;

    if (mode == REPLICA) {
        out << QString("        QVariantList properties;");
        out << QString("        properties.reserve(%1);").arg(astClass.m_properties.size());
        foreach (const ASTProperty &property, astClass.m_properties) {
            out << QString("        properties << QVariant::fromValue(" + property.type() + "(" + property.defaultValue() + "));");
        }
        out << QString("        setProperties(properties);");
    }

    out << "    }";
    out << "public:";

    out << QString("    virtual ~%1() {}").arg(className);

    //First output properties
    foreach (const ASTProperty &property, astClass.m_properties) {
        if (property.modifier() == ASTProperty::Constant)
            out << QString("    Q_PROPERTY(%1 %2 READ %2 CONSTANT)").arg(property.type(), property.name());
        else
            out << QString("    Q_PROPERTY(%1 %2 READ %2 WRITE set%3 NOTIFY on%3Changed)").arg(property.type(), property.name(), cap(property.name()));
    }
    out << "";

    //Next output getter/setter
    if (mode == REPLICA) {
        int i = 0;
        foreach (const ASTProperty &property, astClass.m_properties) {
            out << QString("    %1 %2() const").arg(property.type(), property.name());
            out << QString("    {");
            out << QString("        return propAsVariant(%1).value<%2>();").arg(i).arg(property.type());
            out << QString("    }");
            i++;
            if (property.modifier() != ASTProperty::Constant) {
                out << QString("");
                out << QString("    void set%3(%1 %2)").arg(property.type(), property.name(), cap(property.name()));
                out << QString("    {");
                out << QString("        static int __repc_index = %1::staticMetaObject.indexOfProperty(\"%2\");").arg(className).arg( property.name());
                out << QString("        QVariantList __repc_args;");
                out << QString("        __repc_args << QVariant::fromValue(%1);").arg(property.name());
                out << QString("        send(QMetaObject::WriteProperty, __repc_index, __repc_args);");
                out << QString("    }");
            }
            out << QString("");
        }
    }
    else
    {
        foreach (const ASTProperty &property, astClass.m_properties) {
            out << QString("    virtual %1 %2() const { return _%2; }").arg(property.type(), property.name());
        }
        foreach (const ASTProperty &property, astClass.m_properties) {
            if (property.modifier() != ASTProperty::Constant) {
                out << QString("    virtual void set%3(%1 %2)").arg(property.type(), property.name(), cap(property.name()));
                out << QString("    {");
                out << QString("        if (%1 != _%1)").arg(property.name());
                out << QString("        {");
                out << QString("            _%1 = %1;").arg(property.name());
                out << QString("            Q_EMIT on%1Changed();").arg(cap(property.name()));
                out << QString("        }");
                out << QString("    }");
            }
        }
    }

    //Next output property signals
    out << "";
    out << "Q_SIGNALS:";
    foreach (const ASTProperty &property, astClass.m_properties) {
        if (property.modifier() != ASTProperty::Constant)
            out << QString("    void on%1Changed();").arg(cap(property.name()));
    }

    foreach (const QString &signalName, astClass.m_signals)
        out << QString("    void %1;").arg(signalName);

    if (!astClass.m_slots.isEmpty()) {
        out << "";
        out << "public Q_SLOTS:";
        foreach (const QString &slot, astClass.m_slots) {
            if (mode == SOURCE) {
                out << QString("    virtual void %1 = 0;").arg(slot);
            } else {
                QRegExp re_slotArgs("\\s*(.*)\\s*\\(\\s*(.*)\\s*\\)");
                if (re_slotArgs.exactMatch(slot)) {
                    QStringList types, names;
                    const QStringList cap = re_slotArgs.capturedTexts();

                    const QString functionString = cap[1].trimmed();
                    const QString argString = cap[2].trimmed();

                    if (!argString.isEmpty()) {
                        const QStringList argList = argString.split(',');
                        foreach (QString paramString, argList) {
                            paramString = paramString.trimmed();
                            const QStringList tmp = paramString.split(QRegExp("\\s+"));
                            types << tmp[0];
                            names << tmp[1];
                        }
                    }

                    out << QString("    void %1").arg(slot);
                    out << QString("    {");
                    out << QString("        static int __repc_index = %1::staticMetaObject.indexOfSlot(\"%2(%3)\");").arg(className).arg(functionString).arg(types.join(','));
                    out << QString("        QVariantList __repc_args;");
                    if (names.length() > 0) {
                        QStringList variantNames;
                        foreach (const QString &name, names)
                            variantNames << QStringLiteral("QVariant::fromValue(%1)").arg(name);

                        out << QString("        __repc_args << %1;").arg(variantNames.join(" << "));
                    }
                    out << QString("        qDebug() << \"%1::%2\" << __repc_index;").arg(className).arg(functionString);
                    out << QString("        send(QMetaObject::InvokeMetaMethod, __repc_index, __repc_args);");
                    out << QString("    }");
                }
            }
        }
    }

    //Next output data members
    out << "";
    out << "private:";
    if (mode == SOURCE)
    {
        foreach (const ASTProperty &property, astClass.m_properties) {
            out << QString("    %1 _%2;").arg(property.type(), property.name());
        }
    }

    out << "};";
    out << "";
}
