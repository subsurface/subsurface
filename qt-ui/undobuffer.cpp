#include "undobuffer.h"

UndoBuffer::UndoBuffer(QObject *parent) : QObject(parent)
{

}

UndoBuffer::~UndoBuffer()
{

}

bool UndoBuffer::canUndo()
{

}

bool UndoBuffer::canRedo()
{

}

void UndoBuffer::redo()
{

}

void UndoBuffer::undo()
{

}

void UndoBuffer::recordbefore(QString commandName, dive *affectedDive)
{

}

void UndoBuffer::recordAfter(dive *affectedDive)
{

}
