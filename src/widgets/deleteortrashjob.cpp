/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2022 Ahmad Samir <a.samirh78@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "deleteortrashjob.h"

#include "fileundomanager.h"
#include "widgetsaskuseractionhandler.h"
#include <kio/copyjob.h>
#include <kio/deletejob.h>
#include <kio/emptytrashjob.h>
#include <kio/job.h>
#include <kio/jobuidelegatefactory.h>
#include <kio_widgets_debug.h>

#include <KJobWidgets>

namespace KIO
{

using AskIface = AskUserActionInterface;

class DeleteOrTrashJobPrivate
{
public:
    DeleteOrTrashJobPrivate(const QList<QUrl> &urls, //
                            AskIface::DeletionType deletionType,
                            AskIface::ConfirmationType confirm,
                            QObject *parent,
                            DeleteOrTrashJob *qq)
        : q(qq)
        , m_urls(urls)
        , m_delType(deletionType)
        , m_confirm(confirm)
        , m_parentWindow(qobject_cast<QWidget *>(parent))
    {
    }

    void slotAskUser(bool allowDelete, const QList<QUrl> &urls, AskIface::DeletionType delType, QWidget *parentWindow);

    DeleteOrTrashJob *q = nullptr;
    QList<QUrl> m_urls;
    AskIface::DeletionType m_delType;
    AskIface::ConfirmationType m_confirm;
    QWidget *m_parentWindow = nullptr;
};

void DeleteOrTrashJobPrivate::slotAskUser(bool allowDelete, const QList<QUrl> &urls, AskIface::DeletionType delType, QWidget *parentWindow)
{
    if (!allowDelete) {
        return;
    }

    KIO::Job *job = nullptr;
    switch (delType) {
    case AskIface::Trash:
        Q_ASSERT(!urls.isEmpty());
        job = KIO::trash(urls);
        using UndoMananger = KIO::FileUndoManager;
        UndoMananger::self()->recordJob(UndoMananger::Trash, urls, QUrl(QStringLiteral("trash:/")), job);
        break;
    case AskIface::Delete:
        Q_ASSERT(!urls.isEmpty());
        job = KIO::del(urls);
        break;
    case AskIface::EmptyTrash:
        job = KIO::emptyTrash();
        break;
    }

    if (job) {
        KJobWidgets::setWindow(job, parentWindow);
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
        q->addSubjob(job);
    }
}

DeleteOrTrashJob::DeleteOrTrashJob(const QList<QUrl> &urls, //
                                   AskIface::DeletionType deletionType,
                                   AskIface::ConfirmationType confirm,
                                   QObject *parent)
    : KCompositeJob(parent)
    , d(new DeleteOrTrashJobPrivate{urls, deletionType, confirm, parent, this})
{
}

DeleteOrTrashJob::~DeleteOrTrashJob() = default;

void DeleteOrTrashJob::start()
{
    auto *askHandler = KIO::delegateExtension<AskIface *>(this);
    if (!askHandler) {
        auto *uiDelegate = new KJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled);
        auto *widgetAskHandler = new WidgetsAskUserActionHandler(uiDelegate);
        widgetAskHandler->setWindow(d->m_parentWindow);
        setUiDelegate(uiDelegate);
        askHandler = widgetAskHandler;
    }

    Q_ASSERT(askHandler);

    auto askFunc = [this](bool allowDelete, //
                          const QList<QUrl> &urls,
                          AskIface::DeletionType deletionType,
                          QWidget *window) {
        d->slotAskUser(allowDelete, urls, deletionType, window);
    };
    connect(askHandler, &AskIface::askUserDeleteResult, this, askFunc);
    askHandler->askUserDelete(d->m_urls, d->m_delType, d->m_confirm, d->m_parentWindow);
}

void DeleteOrTrashJob::slotResult(KJob *job)
{
    const int errCode = job->error();
    if (errCode) {
        setError(errCode);
        // We're a KJob, not a KIO::Job, so build the error string here
        setErrorText(KIO::buildErrorString(errCode, job->errorText()));
    }
    emitResult();
}

} // namespace KIO
