/**
 * Copyright 2013-2014 Rene Widera
 *
 * This file is part of libPMacc.
 *
 * libPMacc is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * libPMacc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with libPMacc.
 * If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

#include "eventSystem/EventSystem.hpp"
#include "traits/NumberOfExchanges.hpp"

namespace PMacc
{

    template<class ParBase>
    class TaskReceiveParticlesExchange : public MPITask
    {
    public:

        enum
        {
            Dim = ParBase::Dim,
            Exchanges = traits::NumberOfExchanges<Dim>::value
        };

        TaskReceiveParticlesExchange(ParBase &parBase, uint32_t exchange) :
        parBase(parBase),
        exchange(exchange),
        state(Constructor),
        maxSize(parBase.getParticlesBuffer().getReceiveExchangeStack(exchange).getMaxParticlesCount()),
        initDependency(__getTransactionEvent()),
        lastSize(0) { }

        virtual void init()
        {
            state = Init;
            lastReceiveEvent = parBase.getParticlesBuffer().asyncReceiveParticles(initDependency, exchange);
            initDependency = lastReceiveEvent;
            state = WaitForReceive;
        }

        bool executeIntern()
        {
            switch (state)
            {
                case Init:
                    break;
                case WaitForReceive:

                    if (NULL == Environment<>::get().Manager().getITaskIfNotFinished(lastReceiveEvent.getTaskId()))
                    {
                        state = InitInsert;
                        //bash is finished
                        __startTransaction();
                        lastSize = parBase.getParticlesBuffer().getReceiveExchangeStack(exchange).getHostParticlesCurrentSize();
                        parBase.insertParticles(exchange);
                       // std::cout<<"brecv = "<<parBase.getParticlesBuffer().getReceiveExchangeStack(exchange).getHostCurrentSize()<<std::endl;
                        tmpEvent = __endTransaction();
                        state = WaitForInsert;
                    }

                    break;
                case InitInsert:
                    break;
                case WaitForInsert:
                    if (NULL == Environment<>::get().Manager().getITaskIfNotFinished(tmpEvent.getTaskId()))
                    {
                        state=Wait;
                        assert(lastSize <= maxSize);
                        //check for next bash round
                        if (lastSize == maxSize)
                        {
                            std::cerr << "recv max size " << maxSize << " particles" << std::endl;
                            init(); //call init and run a full send cycle

                        }
                        else
                        {
                            state = Finished;
                            return true;
                        }
                    }
                    break;
                case Wait:
                    break;
                case Finished:
                    return true;
                default:
                    return false;
            }

            return false;
        }

        virtual ~TaskReceiveParticlesExchange()
        {
            notify(this->myId, RECVFINISHED, NULL);
        }

        void event(id_t, EventType, IEventData*) { }

        std::string toString()
        {
            return "TaskReceiveParticlesExchange";
        }

    private:

        enum state_t
        {
            Constructor,
            Init,
            WaitForReceive,
            InitInsert,
            WaitForInsert,
            Wait,
            Finished

        };




        ParBase& parBase;
        state_t state;
        EventTask tmpEvent;
        EventTask lastReceiveEvent;
        EventTask initDependency;
        uint32_t exchange;
        size_t maxSize;
        size_t lastSize;
    };

} //namespace PMacc
